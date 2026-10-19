# Ul'dah interior hwa1 material dispatch boundary

Ul'dah interior `hwa1` has a `LayRaptureMaterial2Clip` on its named hide
side. Original PE RTTI and vtable cells establish that this class inherits
the published `RaptureMaterial2Clip` initializer and application methods.
On the selected factory's nonnull construction branch, the supplied record
pointer reaches runtime object `+0x14` unchanged and supplies ASCII
`modulateColor` to the base constructor's string helper. Its selected
serialized FCurve values and active material effect remain unjoined.

## Evidence identity

The original layout `data/61/5A/00/03.DAT`, layout 431, has 529616 bytes
and SHA-256
`d969e982a55d5c986e7bea24ae5af1bf23bfe7ed18854f7dd7c4f21a11d2353f`.
The [interior owner finding](city-interior-halloween-owner-joins.md#embedded-scheduler-framing-and-exception)
owns the selected timeline, weather-selector, UnitTree and resident joins.
Its hide SCB is `[0x7C310,0x7C870)`, SHA-256
`c231243e707bfdfe8a264309210a459fc70fe2980889af7b0099ca0a86144496`.
All layout offsets here are absolute file offsets and ends are exclusive.

Native locators are VAs in retail 1.23b `ffxivgame.exe`, image base
`0x00400000`, size 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Ghidra 12.1 retained function listings and RTTI were checked against the
original PE using pefile 2024.8.26 and Capstone 5.0.7, x86-32 mode.
The intermediate-constructor, directory-construction, descriptor-selection,
view-construction, string-key and active-receiver observations below use
Python 3.12.0 with those same libraries to decode original PE instructions. Selected SCB structure metadata
was read directly with Python `struct`, without writing asset bytes.
Retained listings supplied locators, and every listed byte, instruction
boundary and full contiguous span in the listed reproduction ranges was
checked against that PE.

## Class and conditional construction

Primary vtable `0x01039F9C` has COL `0x0116C59C` and TypeDescriptor
`0x012CF2F8`, naming
`.?AVLayRaptureMaterial2Clip@Clip@Cut@Scene@Application@@`.
Its hierarchy at `0x0116C5B0` includes base descriptor `0x0116C4F4`,
TypeDescriptor `0x012CF274`, naming
`.?AVRaptureMaterial2Clip@Clip@Cut@Scene@Application@@`.
That base has displacement zero and the same remaining base hierarchy.

The Lay and base primary vtables `0x01039F9C/0x01039E2C` each have 42
slots in `config/ffxivgame.rtti.json`. Original PE cells are identical at
slots 1 through 41. Only slot 0 differs, `0x00835CB0/0x00835B70`.
Both therefore select initializer `0x00835250` at slot 1 and application
`0x00835470` at slot 2, without receiver adjustment at those slots.
The [Material2 ABI](transmission-tower-rendering.md#weather-and-material-overrides)
is applicable to this native subclass under the same initialization and
active-container conditions.

Wrapper `0x008358F0` passes a key read from global `0x012CF16C` and
factory `0x00835B90` to registrar `0x0063F710`. The original global cell
contains pointer `0x010396F8`, whose terminated text is
`LayRaptureMaterial2Clip`. This identifies the key/factory pair supplied
if that wrapper executes while the key retains its original value.

Factory `0x00835B90` writes `0x78` through a nonnull fourth argument
and returns zero on its size-query branch. With fourth argument zero,
it advances the supplied arena cursor by `0x78` and, if the old cursor is
nonnull, calls `0x008350D0` with its first two arguments. That constructor
calls base constructor `0x00834970`, then stores `0x01039F9C` at object
`+0` and `0x01039F90` at object `+8`.

The [SCB builder](scb-clip-factory-composition.md#synchronous-builder-invocation)
supplies `(D,keyEntry,arena,0)` to a selected factory. Its actual selected
registry node and lookup are separate conditions; the wrapper's presence
does not prove selection of `0x00835B90` for this SCB.

The construction argument chain retains the supplied D. The Lay constructor
forwards its first two stack arguments to the same receiver at
`0x008350F8..0x00835104`, and the base constructor forwards them to
`0x00835950` at `0x00834998..0x008349A4`. In that intermediate
constructor, `0x00835972` preserves the incoming ECX in ESI. After its
`0x18` bytes of stack setup, `0x00835978..0x00835984` push the incoming
second and first arguments, then call `0x00A21260` with ECX restored to
that receiver. Generic construction loads the first argument at
`0x00A21260` and stores it unchanged at receiver `+0x14` at
`0x00A21275`. On return, the intermediate writes only receiver `+0` and
`+8`, returns the same receiver, and removes both arguments with `RET 8`
at `0x008359B8`. Its complete body has no other call or branch. Thus
normal construction through this intermediate preserves D for the base
constructor's subsequent read. This does not establish factory selection,
successful initialization or later object state.

## Selected record and inherited consumption

The bounded SCB directory, child extents and linked records identify actor 0
at `0x7C3E0`, size `0x24`, `LayUnitMemberActor`, and actor 1 at
`0x7C404`, size `0x20`, `LaySystemActor`. The one clip D at `0x7C510`
has serialized size `0xA8`, class `LayRaptureMaterial2Clip`, and actor
ordinal 1. The runtime allocation size `0x78` is a different quantity.

`D+0x14` and `D+0x1C` are zero. `D+0x18` is `0x00124F80`, with no
duration or timebase assigned here. `D+0x28` contains `test_aaa`, whose
use remains unknown. `D+0x68` contains `modulateColor`.

At `0x008349EA..0x008349F8`, the base constructor obtains the pointer at
runtime object `+0x14`, adds `0x68`, passes the result to `0x0064E6F0`,
and stores that helper's return at runtime object `+0x5C`. The selected
record thus supplies the `modulateColor` input text on the stated
construction branch.

### Stored name and generated keys

Helper `0x0064E6F0` uses a string-key tree at its receiver `+0x5C`.
That receiver is separate from the material clip. It constructs temporary
byte wrappers from the supplied pointer with `0x00447260`, passing the
length read from global `0x00F67298`. The original cell is `0xFFFFFFFF`,
which selects the published
[NUL-terminated byte-copy contract](map-layout-selector.md). The text
preservation claim requires that value at these calls, terminated input
and successful byte-wrapper construction and copies. Finite-length prefix
cases are outside this full-name contract.

Lookup `0x007EB2F0` uses `0x006534E0` and wrapper comparison
`0x00446EF0` to return a tree/node pair or its end sentinel. Comparison
passes the two wrapper byte pointers and the minimum of their `+8` count
dwords to `0x00445B70`, interpreting a negative result as less-than.
On a miss,
`0x00656C10` constructs a key/value temporary through `0x007E9850`
and supplies it to `0x00656460`. The new-node path uses `0x006553A0`,
`0x00653900` and `0x007EAE70`. The latter constructs the key/value
storage at node `+0x0C` through `0x00669F90`. Both key-copy constructors
call `0x00447200`, whose
[byte-wrapper copy contract](sqwt-markup-font-numeric-properties.md)
retains its allocation and alternate-copy conditions.

At `0x0064E7DF..0x0064E810`, the helper obtains the lookup node again,
checks its owner/end state and passes node `+0x0C` to `0x00445210`.
That accessor returns the dword at wrapper `+0`, its byte pointer. The
returned pointer therefore refers to the retained node's key, rather than
the mapped pair at node `+0x60`. For a newly inserted key successfully
copied from the supplied wrapper, or an existing key with byte-equal
spelling, this preserves the input text. Bounded-comparison equivalence
alone is not promoted to full byte equality of an arbitrary existing key.
No distinct byte allocation or pointer inequality is inferred. Manager and
node lifetime through later application remain conditions.

For application, the precise copy/suffix instructions reload runtime
`+0x5C` independently for each channel. Helper `0x009D29FB` receives
that pointer and a local destination with capacity `0x100`. Its successful
path copies bytes through the first NUL. Each following scan finds that
NUL and replaces it with a two-byte suffix word. Original PE words at
`0x01039928/2C/30/34` are `R`/NUL, `G`/NUL, `B`/NUL and `A`/NUL.

Consequently, if the stated name-storage conditions hold and that pointer's
key storage remains valid and unchanged at application, the generated keys
are `modulateColorR`, `modulateColorG`, `modulateColorB` and
`modulateColorA`. Each occupies 15 bytes including its NUL and fits the
copy buffer. These conditional key bytes do not establish sampled channel
values, the selected active material entry or a visible effect.

The inherited initializer reads `u32(D+0x14)` and calls descriptor
accessor `0x00A20090`. It later requests literal `frc` at `0x010397CC`
through `0x00A2D1B0` and literal `Attr` at `0x01039910`. Its four
selector globals `0x01097E24/28/2C/3C` point to original text
`tx/ty/tz/sx`. Four results are retained at runtime object
`+0x68/+0x6C/+0x70/+0x74`. Application samples those objects and
appends `R/G/B/A` respectively to its runtime `+0x5C` string before
calling each active material entry's slot at byte offset `0x10`.
These runtime offsets are not serialized D offsets or recovered values.

At `0x008353EF`, initialization indexes the actor by `u8(D+3)`, invokes
its slot at byte offset `0x10`, and resolves class text. The comparison
with `LaySystemActor` at `0x0083540C..0x0083541C` sets runtime byte
`+0x50` to 1 on equality. This identifies an explicit system-actor branch
without proving the active native receiver or container.

## Active parameter receivers

Application calls `0x00834A80` while runtime byte `+0x51` is zero
(`0x00835496..0x0083549F`). The caller makes its next call without
testing this helper's return. Let M be the material clip, C the helper's
explicit context argument and D the pointer at M `+0x14`. The helper
passes `u8(D+3)` through `0x00A1FFA0` and `0x00A1B730` to the
[indexed-pointer contract](timeline-weather-record-view.md#synchronous-scheduler-callback).
Its collection is `u32(u32(C+0x0C)+0x70)`. The indexed helper returns
zero for an out-of-range index; `0x00834A80` assumes a valid nonzero
entry before its first virtual call. For the selected record, the supplied
ordinal is 1. This does not establish the entry's concrete native class.

The helper calls that entry's virtual offsets `+0x0C` and `+0x64` in
sequence, ignoring the first return. Let B be the second return. A zero B
exits. For nonzero B, virtual `+0x4C` returns another receiver, whose
virtual `+4` return supplies a mode. That intermediate receiver is not
null-checked. Mode 1 stores B's virtual `+0xBC` return at M `+0x60`.
Mode 2 stores the same virtual call's return at M `+0x64`. Other modes
perform no explicit store to either field.
These mode values come from a virtual return,
separate from initialization's parsed-class flag at M `+0x50`.

The helper then uses those retained candidate fields. If M `+0x60` is
nonzero, its virtual `+0xC4` return contributes a receiver adjusted by
`-4`, or zero when that return is zero. A nonzero M `+0x64` then takes
precedence: its virtual `+0xC4` return replaces the first result after the
same `-4` adjustment. On a zero second return, this body's instructions exit
without falling back to the first candidate or clearing
M `+0x3C`. It also exits if the final adjusted receiver is zero. No explicit
entry store clears either candidate field. Their values at later reads also
depend on the uninspected virtual callees.

For adjusted receiver A, direct helper `0x00623C20` is called with
argument 1. It selects `u32(A+0x0C)` when nonzero, otherwise
`u32(A+0x34)` when nonzero, and calls that selected receiver's virtual
`+0x11C`. With neither field selected, the helper returns zero. A zero
virtual result also
returns zero without trying the other field. A nonzero result J supplies
one further virtual call at `+0x148` in `0x00834A80`. Its return T is
stored at M `+0x3C`. This body's only explicit M `+0x51` store writes 1
for nonzero T. Earlier exits perform no explicit store to M `+0x3C` or
`+0x51`. A zero final T is stored at M `+0x3C` without an explicit
`+0x51` write. A mode-dependent candidate store may already have occurred
before an earlier exit. The helper's explicit instruction sequence
neither rolls back that write nor clears the success byte on failure.
These own-instruction facts do not qualify the unknown callees' state effects.

The application checks `T = u32(M+0x3C)` and skips its parameter loop
when T is zero or `s32(T+0x360)` is nonpositive. It reloads M `+0x3C`
inside the loop. With valid unchanged container storage and signed count
N, the receiver for iteration i is:

```text
E_i = u32(T+0x364) + 0x54*i,  0 <= i < N = s32(T+0x360)
```

Thus `+0x364` is dereferenced to obtain the entry base. The parameter
call uses E_i as ECX and its virtual offset `+0x10`, supplying the
conditional generated key pointer and sampled float. The other three
channel calls use the same entry receiver under the published Material2
ABI. This identifies a conditional receiver expression, not its concrete
type, owning UnitTree, material-context binding, sampled values or visible
effect. The selected `LaySystemActor` class text and captured global
`0x013657B4` do not establish those native joins.

## Directory construction and descriptor selection

The selected directory includes an `@FCURVE` entry at `0x7C630` with
descriptor text `FCurve0000.fcr` at `0x7C660`. The adjacent word at
descriptor `+0x10` is `0x107`; the candidate span
`[0x7C680,0x7C787)` fits the entry, begins `SEDBmtb`, and contains
literal `Attr` at `0x7C782`. These following bytes do not establish the
payload parser or give scalar words consumed channel meanings.

Accessor `0x00A20090` reads its context `+0x0C`, calls `0x00A1CA20`
at `0x00A20098`, then calls `0x00A220A0` with that result as ECX at
`0x00A2009F`. Let `R = u32(context+0x0C)` and `P = u32(R+0x50)`.
At `0x00A1CA35..0x00A1CA3A` and `0x00A1CA81..0x00A1CA91`, the
first helper returns view `V = P+0x1C` when P is nonzero. It neither
reads nor removes a stack argument. The second helper reads the supplied
ordinal i and returns:

```text
u32(V+4) + 16 * u16(u32(V) + 2 + 2*i)
```

The instructions at `0x00A220A0..0x00A220B3` contain no ordinal bound,
null-pointer check or descriptor-content test. Conditional on this
construction and valid view storage, `u32(D+0x14)=0` therefore selects
the descriptor indexed by the first u16 indirection entry, not necessarily
descriptor zero.
When P is zero, `0x00A1CA20` instead returns global view `0x0136510C`,
lazily initialized through `0x00A22010` to three zero dwords. That empty
view does not itself supply valid descriptor storage to `0x00A220A0`.

A checked producer of this view layout is `0x00A27080`. At
`0x00A27095..0x00A270CA`, it loads key pointer `0x01097F94` from
cell `0x01097F9C`. The original eight bytes at `0x01097F94` are
`@FCURVE\0`. It calls `0x00A21F90` with receiver `Q+8`, where Q is its incoming ECX. For a
nonzero return T, it constructs a temporary view with `0x00A22020`
and copies its first two dwords to `Q+0x1C` through `0x00A22080`.
The resulting storage contract is:

```text
u32(Q+0x1C) = T
u32(Q+0x20) = T + ((2*u16(T) + 0x11) & ~0x0F)
```

Constructor `0x00A230B0` initializes the view at `Q+0x1C` with
`0x00A22010`. `0x00A24E70` constructs Q at `0x00A24F5B`, calls
`0x00A27080` on Q at `0x00A2509C`, and can return Q at `0x00A2514D`.
This is a concrete construction path, not proof that Q becomes the P read
by the selected material context.

For directory receiver E, `0x00A21F90` uses `s32(E+4)` as the entry
count and visits entries from j=0 while j is below that count. Candidate j is
`u32(u32(E+8)+4*j) + u32(E+0x0C) + u32(E+0x10)`.
It calls `0x009D5475` with `(candidate,key,8)` and returns
`candidate+0x10` on the first zero result, or zero if the loop finishes.
The checked comparator performs a case-sensitive comparison of at most the
supplied count of bytes. It returns the unsigned-byte difference on a
mismatch, or zero at matching NUL bytes or count exhaustion. The complete
original body has no further call.

The directory fields are constructed by `0x00A26D70`. In the caller's
accepted-version branch, let Z be its incoming receiver,
`W = u32(Z+0x50)` and `C = W+u16(W+0x0E)`. At
`0x00A24FCB..0x00A24FD8`, the caller supplies `(C,1)` to local view
constructor `0x00A27270`, which stores C at local `+4` and 1 at `+8`.
Getters `0x00A272B0`, `0x00A272F0` and `0x00A27310` then provide
`B = C+0x10`, the low word at C+`0x0A`, and `L = 16*u16(C+6)-0x10`.
The caller sign-extends that word at `0x00A25007`, producing
`N = s16(C+0x0A)`. It pushes `(N,B,L,0x10)`
and calls `0x00A26D70` on Q at `0x00A2500D`.

At `0x00A26D88..0x00A26DB7`, that helper uses `0x00A21ED0` to
construct a temporary directory and `0x00A21F30` to copy its four data
fields to `E = Q+8`. Cleanup `0x00A21F00` writes only the temporary's
vtable cell. The retained directory fields are:

```text
s32(E+4)    = N
u32(E+8)    = B
u32(E+0x0C) = B+L = C+16*u16(C+6)
u32(E+0x10) = 0x10
```

Thus candidate j is `J[j]+0x10`, with
`J[j] = C+16*u16(C+6)+u32(C+0x10+4*j)`. J denotes a serialized
SCB directory entry here, separate from a factory class-key table.
The adjacent `0x00A26DE0` call at `0x00A2507B` stores a distinct result
at Q `+0x38` in its checked body. It is not the directory field producer.
The subsequent view-population call remains `0x00A27080` on Q at
`0x00A2509C`.

For the selected hide SCB, C is file `0x7C340`, `u16(C+6)=3`, N is 6,
B is `0x7C350` and B+L is `0x7C370`. Directory entry j=2 has relative
offset `0x2C0`, so `J[2]=0x7C630` and its candidate key is at
`0x7C640`. The first two entries' keys are `schedul` and `RIDTBL`.
Entry 2 is the first eight-byte `@FCURVE\0` match and returns
`T = 0x7C650`. At T, the u16 descriptor count is 1 and the u16 ordinal-zero
indirection at T+2 is 0. The aligned descriptor base is therefore
`T+0x10 = 0x7C660`.

Conditional on this selected SCB being supplied to the checked construction
path and the material accessor's P being Q, ordinal zero returns descriptor
`0x7C660`, whose text is `FCurve0000.fcr`. This establishes descriptor
selection under those conditions, without identifying scalar values or
proving that the following candidate payload is consumed.

## Exact remaining edge

The material accessor's actual context-to-Q binding remains unjoined. The
[SCB factory path](scb-clip-factory-composition.md#parsed-clip-key-table)
joins P to Q for its fireworks scheduler construction; that separate path
does not identify this material context. The selected directory and
indirection bytes do not establish that the material accessor reaches this
constructed Q, the downstream payload parser, or FCurve scalar consumption.
No serialized Attr values are assigned.

FCurve scalar construction helpers and registry lookup remain conditions.
The [active parameter receiver chain](#active-parameter-receivers) still
requires the actual virtual results and valid retained container storage.
Base construction captures current global
`0x013657B4` into runtime `+0x54`; that does not identify owning UnitTree U
or prove context preservation. Static class and parameter evidence does
not establish historical execution, a native hide operation, a visible
material effect or backend/server policy.

## Reproduction

Check the exact input identities. Follow the SCB directory, class strings,
actor links and clip links using the [SCB record framing](cut-scheduler-clip-layout.md).
Read the original COL, hierarchy descriptors and 42 cells of each named
primary vtable. Check the full original PE instruction bytes, boundaries,
calls and cited operands for these exclusive ranges:

| Purpose | Start VA | Stop VA |
| --- | --- | --- |
| Base constructor | `0x00834970` | `0x00834A26` |
| Lay constructor | `0x008350D0` | `0x0083513B` |
| Lay cleanup | `0x00835140` | `0x0083518F` |
| Base initializer | `0x00835250` | `0x00835464` |
| Base application | `0x00835470` | `0x00835733` |
| Registration wrapper | `0x008358F0` | `0x0083591D` |
| Intermediate constructor | `0x00835950` | `0x008359BB` |
| Lay factory | `0x00835B90` | `0x00835C18` |
| Descriptor accessor | `0x00A20090` | `0x00A200A7` |
| Generic D retention | `0x00A21260` | `0x00A212A8` |
| Descriptor view selection | `0x00A1CA20` | `0x00A1CA92` |
| Ordinal descriptor lookup | `0x00A220A0` | `0x00A220B4` |
| Empty view constructor | `0x00A22010` | `0x00A2201D` |
| Serialized view constructor | `0x00A22020` | `0x00A22041` |
| View pointer assignment | `0x00A22080` | `0x00A22093` |
| Temporary view cleanup | `0x00A22070` | `0x00A22071` |
| Directory selection | `0x00A21F90` | `0x00A21FD8` |
| View owner constructor | `0x00A230B0` | `0x00A2311A` |
| View population | `0x00A27080` | `0x00A271A1` |
| View construction path | `0x00A24E70` | `0x00A25168` |
| Directory construction | `0x00A26D70` | `0x00A26DDF` |
| Separate Q+0x38 construction | `0x00A26DE0` | `0x00A26E4D` |
| Directory field constructor | `0x00A21ED0` | `0x00A21EF7` |
| Directory field assignment | `0x00A21F30` | `0x00A21F51` |
| Temporary directory cleanup | `0x00A21F00` | `0x00A21F07` |
| Bounded byte comparison | `0x009D5475` | `0x009D5533` |
| Chunk view constructor | `0x00A27270` | `0x00A27289` |
| Chunk table pointer | `0x00A272B0` | `0x00A272C0` |
| Chunk count | `0x00A272F0` | `0x00A272F8` |
| Chunk extent minus prefix | `0x00A27310` | `0x00A2731E` |
| Stored-name tree helper | `0x0064E6F0` | `0x0064E82B` |
| Name lookup result | `0x007EB2F0` | `0x007EB355` |
| Mapped-value access and insertion | `0x00656C10` | `0x00656CDB` |
| Name lower-bound search | `0x006534E0` | `0x00653519` |
| Wrapper ordering | `0x00446EF0` | `0x00446F1D` |
| Temporary key/value construction | `0x007E9850` | `0x007E98B6` |
| Hinted name insertion | `0x00656460` | `0x00656650` |
| Name node insertion | `0x006553A0` | `0x0065558D` |
| Name node allocation forwarding | `0x00653900` | `0x0065398F` |
| Name node construction | `0x007EAE70` | `0x007EAEE6` |
| Node key/value copy | `0x00669F90` | `0x00669FF5` |
| Bounded application copy | `0x009D29FB` | `0x009D2A60` |
| Active-container selection | `0x00834A80` | `0x00834B52` |
| Adjusted receiver field selection | `0x00623C20` | `0x00623C58` |
| Context forwarding | `0x00A1FFA0` | `0x00A1FFA8` |
| Actor collection forwarding | `0x00A1B730` | `0x00A1B738` |

For the D-retention edge, check the full intermediate-constructor range and
reuse the prior Lay/base forwarding and generic-store observations. The
precise supporting snippets are `[0x008350D0,0x00835109)`,
`[0x00834970,0x008349A9)`, `[0x00A21260,0x00A21278)` and
`[0x00A212A5,0x00A212A8)`. These include the forwarding prologues,
D store and generic stack cleanup.

For directory construction, the reused caller snippets are
`[0x00A24F66,0x00A24F7C)`, `[0x00A24FCB,0x00A25012)`,
`[0x00A2505F,0x00A25080)` and `[0x00A25095,0x00A250A1)`.
They identify the source span, directory arguments, separate Q+0x38 call
and view-population receiver. Reuse the published directory selection and
view-alignment observations, then read only the selected SCB's header,
directory entries, count, first indirection word and descriptor text to
reproduce the file-offset result.

For stored-name construction and application, the reused material snippets
are `[0x008349DD,0x008349FB)`, `[0x008355B0,0x008355EF)`,
`[0x00835601,0x00835639)`, `[0x0083564A,0x00835680)` and
`[0x00835691,0x008356C9)`. They connect the helper receiver, supplied
record text, returned pointer, bounded copies and suffix stores. Reuse the
published byte-wrapper construction/copy/destruction and bounded-comparison
contracts. Check pointer accessor `[0x00445210,0x00445213)`, the original
length cell and the four suffix words directly in the PE.

For active parameter receivers, reuse the published indexed-pointer
contract at `0x00A21C80`. The precise material application operands are
`[0x00835496,0x008354AB)`, `[0x00835557,0x0083557C)`,
`[0x00835580,0x0083558B)`, `[0x008355B3,0x008355BD)`,
`[0x008355EF,0x00835601)` and `[0x008356DA,0x008356EC)`.
They establish the cached-success gate, container/count checks,
dereferenced entry base, parameter receiver and stride. No concrete
virtual table is assigned to the returned receivers.

These selected bodies are contiguous. The exporter's `# size` generally
sums function-body AddressSet ranges; it does not promise an enclosing
span without gaps. The named input and native checks establish static
data and conditional own-body behavior, with no client execution.
