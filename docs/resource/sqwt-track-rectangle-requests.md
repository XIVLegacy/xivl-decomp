# Sqwt Track and ScrollBar rectangle requests

The native Track arrange override computes a Thumb rectangle from supplied
Track fields, a final size and the child's desired size. Its desired-height
branch recomputes the rectangle rather than enforcing a minimum height. This
page owns those conditional requests and the ScrollBar requests that precede
them. Final layout and rendering remain UNKNOWN.

The [Sqwt parts study](sqwt-tab-scroll-text-parts.md) owns the Control layout
rules, style parts and unresolved-background questions. STATIC below means an
instruction, string or RTTI observation. Property roles inferred from descriptor
and name associations are identified separately.

## Executable and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, PE32 image
base `0x00400000`. All addresses on this page are VA.

The producing instruments are Ghidra through `ghidra-cli` for focused
disassembly, cross-references and decompiler leads, and Node Buffer reads for
independent PE section mapping, instruction-byte comparison, relative-target
calculation and RTTI walks. The observations below derive from instructions
and data. Decompiled types and expressions are leads, not original source.
These are static observations, not target execution or live layout results.

| Function | Inclusive VA range |
|---|---|
| Track arrange | `0x0099BD90..0x0099BFD9` |
| ScrollBar measure | `0x00989D60..0x00989EE4` |
| ScrollBar arrange | `0x00989EF0..0x0098A0AD` |
| Vertical part name assignment | `0x00970B30..0x00970D18` |
| Track constructor | `0x0099C460..0x0099C678` |
| Track inherited measure | `0x0092A250..0x0092A2F4` |
| Arrange request helper | `0x0093A870..0x0093AAA1` |

The complete Track arrange body is `586` bytes with SHA-256
`ad91719bb6767be6375a26cda8a3e11332e0ce95c0541d1882c3b9e139c043fb`.

## Classes and vertical part path

The offset-0 vftables have the layout slots below. STATIC. The RTTI catalogs
use RVA, so add the image base before comparing them with these VAs:
[RTTI](../../config/ffxivgame.rtti.json) and
[vtable slots](../../config/ffxivgame.vtable_slots.jsonl).

| Class | Offset-0 vftable | Arrange slot 54 | Measure slot 55 |
|---|---|---|---|
| ScrollBar | `0x0107798C` | `0x00989EF0` | `0x00989D60` |
| Track | `0x0107A54C` | `0x0099BD90` | `0x0092A250` |
| Thumb | `0x0108488C` | `0x00937480` | `0x00937310` |

The type descriptor names are
`.?AVScrollBar@Primitives@Controls@Sqwt@@`,
`.?AVTrack@Primitives@Controls@Sqwt@@` and
`.?AVThumb@Primitives@Controls@Sqwt@@`. Their type descriptors are
`0x012E60D0`, `0x012E7934` and `0x012EA47C`, respectively. The COL offset
is zero for each table above. Other subobject tables do not supply the layout
identities in this table.

The name assignments in `0x00970B30..0x00970D18` identify these members.
STATIC.

| Owner | Member | Assigned name | Name string VA |
|---|---|---|---|
| ScrollViewer | `+0xC78` | PART_VerticalScrollBar | `0x010730AC` |
| ScrollBar | `+0x384` | PART_UpButton | `0x010730C4` |
| ScrollBar | `+0x388` | PART_DownButton | `0x010730D4` |
| ScrollBar | `+0x380` | PART_Track | `0x010730E4` |
| Track | `+0x2C8` | PART_Thumb | `0x010730F0` |

The UpButton, DownButton and Track assignments guard null members. The Thumb
assignment has no separate null test. These names establish this object path,
not its current owner, lifetime or live state.

## Track input associations

The constructor associates the following fields with descriptors. The
initializer ranges associate those descriptors with the named strings. The
addresses and associations are STATIC. Interpreting the fields as the
corresponding properties follows those associations.

| Track field | Name | Descriptor | Name string VA | Initializer range |
|---|---|---|---|---|
| `+0x2A4` | Maximum | `0x01360EE4` | `0x0107A388` | `0x00F284A0..0x00F284C3` |
| `+0x2AC` | Minimum | `0x01360F50` | `0x0107A390` | `0x00F284D0..0x00F284F3` |
| `+0x2B4` | Value | `0x01360F74` | `0x0107A3A4` | `0x00F28530..0x00F28553` |
| `+0x2BC` | ViewportSize | `0x01360F08` | `0x0107A3AC` | `0x00F28560..0x00F28583` |
| `+0x2C4` | Orientation byte | `0x01360F2C` | `0x0107A398` | `0x00F28500..0x00F28523` |

The constructor passes `1.0` for Maximum, `0.0` for Minimum, Value and
ViewportSize, and byte `1` for Orientation to the respective property helpers.
These helper arguments do not establish universal effective defaults. It sets
`+0x2C8` to `this+0x2CC` and calls `0x009CA650` at that member. STATIC.

Arrange selects width-axis arithmetic for Orientation byte zero and
height-axis arithmetic for any nonzero byte. The flag at `+0x2C6` bit
`0x20` is described by its formula below. Its property name is not established.

## Height-axis Track request

The following inputs describe the height-axis branch of
`0x0099BD90..0x0099BFD9`:

- Maximum `M`, Minimum `m`, Value `v` and ViewportSize `V`.
- Nonzero Orientation and flag `b = (+0x2C6 & 0x20) != 0`.
- Supplied final size `(W,L)`.
- Child pointer `p = [Track+0x2C8]` and desired height
  `h = float[p+0x12C]`.

The ordered finite algebra below summarizes the observed arithmetic. It is
not a bit-exact floating-point model.

1. Select `q = M-v` when `b` is set, otherwise `q = v`.
2. Compute `R = M-m` and store `D = V+R` as a float.
3. Start `r` at zero. If stored `D` is not ordered-equal to zero, recompute
   `V/(V+R)` and store it as float `r`.
4. Start `T` at `L`. If the float-stored difference `r*L-h` is negative,
   recompute `T = (L-h)/(R/D)` and store it as a float.
5. Start `y` at zero. If stored `D` is not ordered-equal to zero, compute
   `T*((q-m)/D)` and store it as float `y`.
6. Submit rectangle `(0,y,W,T*r)` to the child through `0x0093A870`.
   Copy `(W,L)` unchanged to the output size.

STATIC for the reads, arithmetic, stores and branches. The reversed
`q=M-v` is float-stored before the later subtraction of `m`. The formula
is not `M+m-v`. `R` remains an x87 intermediate. The ratio recomputes
`V+R` rather than simply reusing the float-stored `D`. The desired-height
difference is float-stored before comparison. Adjusted `T` and `y` are
float-stored, and the requested height uses `T*r`. Those stores and x87
intermediates prevent treating the algebra as exact floating-point equivalence.

The desired-height branch is a recomputation, not a conventional minimum clamp.
As conditional finite symbolic arithmetic, `M=80`, `m=0`, `V=20`,
`L=100` and `h=30` give `T=87.5` and requested height `17.5`.
This is an unexecuted algebra example, not a runtime result. Replacing that
branch with `max(h,proportionalHeight)` does not describe these instructions.

### Comparison and refusal limits

The guards at `0x0099BDFD..0x0099BE06` and
`0x0099BF5A..0x0099BF63` use FUCOM, FNSTSW, a test of AH against
`0x44`, and JNP. Ordered zero skips division. Unordered takes division
if execution proceeds with masked floating exceptions. STATIC.

At `0x0099BF21..0x0099BF2A`, FCOM compares zero with the float-stored
difference `r*L-h`. When execution proceeds with masked floating exceptions,
FNSTSW, a test of AH against `0x41`, and JNZ skip recomputation for equal,
positive or unordered stored difference. A negative stored difference takes
the recomputation. The floating-control environment is UNKNOWN.

There is no additional `R==0` or `D==0` guard inside the recomputation,
no finite-value rejection and no general clamp of Value, range, viewport,
position or length in this body. The child pointer is dereferenced without a
null check. These absences establish no safe domain, exception guarantee or
downstream acceptance of negative or nonfinite rectangles.

### Conditional extent substitution

The upstream extent/viewport/offset binding is not traced here. As conditional
ordered finite algebra only, suppose Orientation is nonzero, `+0x2C6` bit
`0x20` is clear, `m=0`, `M=extent-viewport`, `V=viewport`,
`v=offset`, extent is nonzero, and the desired-height branch leaves `T=L`.
The requested thumb height then reduces to `L*viewport/extent` and `y`
to `L*offset/extent`. With that flag set instead, the same other assumptions
give `y=L*(extent-viewport-offset)/extent`. These substitutions are not a
bit-exact floating-point model or evidence of an actual upstream state binding.

## ScrollBar measure and Track request

The custom branch of ScrollBar measure `0x00989D60..0x00989EE4` and
arrange `0x00989EF0..0x0098A0AD` requires `+0x38C` bit `0` to be set.
When that bit is clear, the functions delegate to the Control pair described
in the [parts study](sqwt-tab-scroll-text-parts.md#layout-rules). STATIC.

Custom measure passes the same supplied available size to each non-null child
at `+0x384`, `+0x388` and `+0x380` through `0x0093CD90`. Absent
children contribute zero. For nonzero Orientation at `+0x37C`, ordered
child comparisons select the maximum desired width and the result height is
the sum of desired heights. Those components are read at `+0x128/+0x12C`.
This finite algebra does not define nonfinite inputs as ordinary maxima.

Custom arrange submits `(0,0,W,H)` to each non-null end button at
`+0x384` and `+0x388` through `0x0093A870`. After each request it
reads `+0x1B4` and `+0x1AC` from that child. With nonzero Orientation,
it submits `(0,a,W,H-a-b)` to the non-null Track at `+0x380`, where
`a` and `b` are the first and second `+0x1AC` post-request scalar
reads. Missing child contributions remain zero. It returns `(W,H)` unchanged.
STATIC. There is no clamp of `H-a-b` in this body.

The scalar reads are inputs to this request. Their broader semantic identity
and the end buttons' final dimensions and positions remain UNKNOWN. The
full-rectangle requests do not establish overlapping final button placement.

Track's inherited measure at `0x0092A250..0x0092A2F4` seeds its result
from virtual slot `39` at `+0x9C`, enumerates children through `+0x254`,
and measures each with the supplied available size through `0x0093CD90`.
Each result component is raised only when the child desired component at
`+0x128/+0x12C` is greater. STATIC. The seed and child desired sizes
remain inputs. Thumb inherits Control arrange and measure, with no separate
geometry override at slots `54/55`.

## Arrange request boundary

The helper `0x0093A870..0x0093AAA1` substitutes a shared zero rectangle
when Visibility byte `+0x158` is zero. It checks pending layout and cached
rectangle fields. Its core path stores the supplied components into
`+0x168/+0x16C/+0x170/+0x174` when visibility is nonzero and dispatches
virtual slot `26` at `+0x68`. A cache path can skip that dispatch when
dimensions are unchanged and update position through `0x00940110`.
The helper itself has no finite or positive clamp. STATIC.

The downstream slot-26 core and `0x00940110` behavior remain UNKNOWN.
Effective constraints, margins, alignment, pixel rounding, final geometry,
current owner and lifetime, live state and rendering are not established by
these rectangle requests.

The [parts study's unresolved-part question](sqwt-tab-scroll-text-parts.md#open-questions)
also remains open. The missing VerticalBackground resolver/default-record/draw
consumer join and the eight float roles and three pointer roles in the default
records are UNKNOWN. This page establishes no fallback or no-draw result.

## Reproduction

Use an executable with the stated size and SHA-256. Read its PE32 section
headers and map each cited VA by subtracting `0x00400000` and locating the
corresponding section's raw data. Walk each vftable's preceding COL, confirm
its zero offset and type descriptor, and compare slots `54/55` with the
public RTTI and slot catalogs.

Use focused Ghidra disassembly of the complete inclusive ranges above.
For example, `ghidra disasm 0x0099BD90 -n 250 --json` includes the full
Track arrange body. Limit interpretation to that body's end at
`0x0099BFD9`. Independently compare instruction bytes and relative branch
destinations with the mapped executable. Read the five property-name strings
and follow their initializer descriptor associations into the constructor.
Preserve the x87 store and comparison order when evaluating the conditional
algebra. Final layout requires separate evidence from the downstream paths.
