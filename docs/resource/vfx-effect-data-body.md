# Qix veff data body

The [effect family](vfx-effect-family.md) page records the veff root name, the
reference arrays and the sub-record start. This page records the rest of the
root directory: the control, link, unit and instance arrays, the control slot
records, the unit graph records with their polynomial segments, the link codes
and the lifetime, bounds and culling words. The module names that the controls
declare are on the [control module registry](vfx-control-module-registry.md)
page and the frame update is on the [effect runtime](vfx-effect-runtime.md) page.

The machine-checked form of every STATIC fact is
[`vfx_effect_runtime.json`](../../config/vfx_effect_runtime.json), checked by
`tools/verify_vfx_effect_runtime.py`. The parser is `tools/vfx_effect_body.py`.
The identity and method section of the
[container page](vfx-qix-resource-container.md#identity-and-method) applies
here unchanged, and so do its tier labels.

## Root directory

Offsets are relative to root record 0 of a veff blob. Data Effect creation reads
the root through Create_Static at `0x00bd3f70`. Arrays are a payload relative
pointer followed by a u32 count.

| Offset | Width | Field | Evidence |
|---|---:|---|---|
| `+0x00` | 2 | version, exactly `0x1d` | STATIC, `0x00bd3f94` |
| `+0x1c` | 2 | graph A record total | STATIC, sizes storage at `0x00bd41bb` |
| `+0x1e` | 2 | graph B record total | STATIC, same |
| `+0x30` | 8 | control array, 0x24 byte entries | STATIC, `0x00bd4027` |
| `+0x38` | 8 | link array, 0x1c byte entries | STATIC, `0x00bd409f` |
| `+0x40` | 8 | unit array, 0x38 byte entries | STATIC, `0x00bd41e7` and `0x00bd4551` |
| `+0x48` | 8 | instance array, 0x48 byte entries | STATIC, `0x00bd4271` |
| `+0x50` | 4 | lifetime in ticks, `0x7fffffff` is endless | STATIC, `0x00bd2cd0` |
| `+0x54` | 4 | source path offset | STATIC, kill log at `0x00bc7008` |
| `+0x58` | 8 | declaration table, 24 byte entries | STATIC, `0x00bd4032` |
| `+0x70` | 12 | bounds minimum | STATIC, `0x00bd32c0` |
| `+0x80` | 12 | bounds maximum | STATIC, same |
| `+0x90` | 1 | culling type, 0 to 3 | STATIC, `0x00bd38d0` |
| `+0x94` | | leaf sub-record | see the effect family page |
| `+0x100` | 2 | shader parameter hint count | STATIC, `0x00bd47c0` |

The installed effects of version `0x1d` use culling types 0 (225) and 1 (844).
None is endless. The corners at `+0x70` and `+0x80` are equal in 1,031 of the
1,069, and the code replaces equal corners with a default box. The source path
is a printable NUL terminated string in all 1,069. The words at `+0x60` to
`+0x6c` are not read here and are UNKNOWN.

The Context constructor `0x00bad5d0` stores the address of a built-in record,
`0x01303e68`, at context `+0x24` (accessor `0x00bd3090`). It has the version
word `0x1d`, the lifetime `0x7fffffff` at `0x01303eb8`, one unit and one
instance, and no controls. CreateRootEffect_ at `0x00bae0a0` requires the three
leaf module names that sit as string pointers at `0x01303e24` (`LeafLife`,
`MatrixLeaf` and `ColorRGBALeaf`) to resolve. The source path pointer at `+0x54`
points to `Data::Effect::Default`. The context root effect and the failed-root
fallback are built from this record.

## Array entries

The tables below give each entry layout. Offsets are relative to the entry.

### Control entry (0x24 bytes)

| Offset | Width | Field | Evidence |
|---|---:|---|---|
| `+0x00` | 8 | slot record list, 16 byte records | INFERRED |
| `+0x10` | 8 | second list | INFERRED |
| `+0x18` | 4 | parameter block pointer | INFERRED |
| `+0x1c` | 2 | declaration index, rewritten to the module index | STATIC, `0x00bd4027` |

### Link entry (0x1c bytes)

| Offset | Width | Field | Evidence |
|---|---:|---|---|
| `+0x00` | 8 | static link codes (u32 each) | STATIC, `0x00bdbf10` |
| `+0x08` | 8 | expression link codes (u32 each) | STATIC, same |
| `+0x14` | 2 | control index | STATIC, `0x00bd409f` |

### Unit entry (0x38 bytes)

| Offset | Width | Field | Evidence |
|---|---:|---|---|
| `+0x00` | 8 | control node list, u16 link indices | INFERRED |
| `+0x08` | 8 | child instance list, u16 instance indices | INFERRED |
| `+0x10` | 8 | child schedule, 8 byte records | STATIC, `0x00bce7f7` |
| `+0x18` | 8 | graph A list, 12 byte records | STATIC, `0x00bd41bb` sizes |
| `+0x20` | 8 | graph B list, 12 byte records | STATIC, same |
| `+0x28` | 4 | | UNKNOWN |
| `+0x2c` | 4 | read by the length pass when the end byte is `0xff` | STATIC, `0x00bd2d0d` |
| `+0x30` | 4 | | UNKNOWN |
| `+0x34` | 1 | end byte, `0xff` in every unit | STATIC, `0x00bd2d0d` |

A schedule record is an s32 start tick, a u16 index into the child instance list
and a u16 zero. GenerateChildUnit_ at `0x00bce770` creates the child when the unit
time reaches the start tick. The words at unit `+0x28` and `+0x30` hold tick
values such as 210000 and 300000 in the installed units and 0x7fffffff in the
root unit. Their roles are UNKNOWN.

### Instance entry (0x48 bytes)

| Offset | Width | Field | Evidence |
|---|---:|---|---|
| `+0x00` | 2 | unit index | STATIC, `0x00bd2cf3` |
| `+0x08` | 1 | draw layer | INFERRED, read by `0x00bd38d0` |
| `+0x0f` | 1 | flags, bit 1 tested by the length pass | STATIC, `0x00bd2cf3` |

### Slot record (16 bytes)

The control node builder `0x00bdb9d0` walks the slot records of a control entry.
Each record names a value source in the graphs of the unit that owns the control.

| Offset | Width | Field | Evidence |
|---|---:|---|---|
| `+0x04` | 1 | value kind | INFERRED |
| `+0x09` | 1 | graph index into the unit graph A list | STATIC, `0x00bdba70` |
| `+0x0a` | 1 | binding index in that graph record | STATIC, `0x00bdba70` |
| `+0x0b` | 1 | `0xff` or an index into the unit graph B list | STATIC, `0x00bdba70` |

The builder stores one value pointer per slot at the work base plus descriptor
`+0x2c`, for slots below the count at descriptor `+0x30`. In the 122,722 installed
slot records the graph index is below the graph A count of the owning unit and
the binding index is below the binding count of that record in every case. The
random index is `0xff` in 115,723 and a valid graph B index in 6,999. INFERRED
from the install, STATIC for the reads.

## Link codes

A link code is a u32 in a link entry list. ControlNode::ResolveStaticLink at
`0x00bdbf10` resolves it by the top nibble. STATIC.

| Top nibble | Meaning |
|---|---|
| `0` | unit local. Bits 16 to 27 pick a unit definition and the low 16 bits pick a control node of that unit. The result is that node's work base plus 0x10 |
| `1` | family. Bits 16 to 27 pick a declaration. The nodes of the unit, then its parents, are searched for the module or one derived from it, then the descriptor provider slot `+0xbc` is tried, then the default target `+0xd8` |
| `2` | fixed. Matched by module index first, then the family search with a warning |
| `3` | rejected with `not resolve static-link code` |

At most seven static links are accepted per control (`static-link max over`).
Descriptor `+0x34` gives the storage offset of the static link results in the
work buffer and `+0x3c` the storage offset of the expression link results.

## Unit tree

In each of the 1,069 current effects the instance array forms a tree. INFERRED
from the install, with the reads above as the STATIC base.

- The unit and instance arrays have equal counts, and instance 0 names unit 0.
- Instance 0 appears in no child list and every other instance appears in
  exactly one, so the lists are a spanning tree rooted at instance 0.
- The control and link arrays have equal counts, the link control index is a
  permutation, and the sum of the unit control node list counts equals the link
  count.
- The graph A and graph B totals at root `+0x1c` and `+0x1e` equal the sums of
  the unit list counts at `+0x1c` and `+0x24`.
- Every unit list index is in range, every schedule index is below its child
  list count, schedule start ticks do not decrease, the pad words are zero and the
  unit end byte is `0xff`.
- The array totals are 54,358 controls, 54,358 links, 10,235 units and
  10,235 instances.

## Graph records and polynomial segments

A unit graph record is 12 bytes: a u16 module index, a byte at `+0x02` that is the
overflow id (0 or 1), a byte that is zero in the install, a pointer to the
bindings at `+0x04` and a u16 binding count at `+0x08`. The module index selects a row of the standard
table for list A and of the random table for list B. STATIC, `0x00bd28d6`,
`0x00bd2a86`, `0x00be7d50` and `0x00be7ce0`. A graph overflow id above 1 logs
`graph overflow id error`.

A binding is three u32 values. INFERRED from the install.

| Offset | Field |
|---|---|
| `+0x00` | offset of a u32 cell that holds the offset of the first segment |
| `+0x04` | offset of one u32 start tick per segment |
| `+0x08` | segment count |

A polynomial segment is a degree byte, a component byte, two padding bytes and
`components * (degree + 1)` f32 coefficients, component major with the constant
term first. The next segment follows with no padding. STATIC for the layout, from
the init routine at `0x00bec270`. The evaluator at `0x00bec2c0` computes per
component `c0 + c1 * x + 0.5 * c2 * x^2` and adds a carried value, and the
routine at `0x00bec4d0` gives the derivative `c1 + c2 * x`. The components byte
equals the width in the module name. STATIC for the following evaluator details:

- The init routine writes `c0` of each component to the output and zeroes the
  carried value, which sits at output plus 4 * (components + 1).
- The evaluator returns at once when the degree byte is 0, so a degree 0 segment
  keeps the init value and never gets the carried value added.
- Every term of degree 2 or higher is scaled by the same 0.5 rather than by
  `1 / k!`, so the formula above holds as written only for degree 2 or lower.

In the install the 51,929 graph records hold 107,656 bindings and 157,458
segments, and every binding walks as a contiguous run of segments with a matching
component count. Degrees are 0 (84,118 segments), 1 (7,505) and 2 (65,835). The
start tick array begins at 0 and does not decrease in all bindings except 10.
INFERRED. The unit of x, the way x relates to the start ticks and the use of the
carried value are not read, see the boundaries.

## Boundaries

- Control parameter blocks, the second control list and the 16 byte slot value
  kinds are not decoded. A control with a block pointer at the payload end has no
  block.
- The meaning of unit `+0x28` and `+0x30`, of root `+0x60` to `+0x6c` and of the
  link entry fields beyond the lists is UNKNOWN.
- The Genesis graph forms spawn values over time and are read only as far as the
  polynomial exponent byte of the spawn count routine at `0x00bed1b0`. They are
  not followed here.
- No effect was played back and nothing here shows a rendered result.

## Verification

```text
python tools/verify_vfx_effect_runtime.py --exe orig/ffxivgame.exe
python tools/verify_vfx_effect_runtime.py --install <game-root>
python tools/test_vfx_effect_runtime.py
```

`--install` recomputes the array totals, the tree relations, the graph and slot
counts and checks them against the manifest. The tests build a synthetic veff with
a unit tree and compare the census with literal expected counts.
