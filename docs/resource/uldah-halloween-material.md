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
view-construction, string-key, initializer-context, channel-producer,
sample-input and active-receiver observations below use Python 3.12.0 to decode
original PE instructions. Selected SCB structure metadata
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

### Initializer context

Initializer `0x00835250` receives the clip M in ECX and a caller-supplied
context `C_init` as its first stack argument. The prologue reserves
`0x28` bytes before `0x00835279` reads that argument from ESP `+0x2C`
into EBP. It calls `0x00A20F30` with receiver M and explicit argument
`C_init`. At `0x00835283..0x0083528C`, it then loads D from M `+0x14`,
pushes `u32(D+0x14)` and uses EBP as the receiver for descriptor accessor
`0x00A20090`. Under the normal callee-preserved-register convention, the
accessor therefore receives the supplied `C_init`; the base initializer's
EAX return is not used as that context pointer.

Base initializer `0x00A20F30` first reads key word
`u32(u32(M+0x18))` and supplies it to `0x00A20110` with receiver
`C_init`. That forwarding body reads `R0 = u32(C_init+0x0C)` and
`L = u32(R0+0x84)`, then tail-jumps to `0x00A21680` with receiver L
and the same explicit key word. Its EAX return is stored at M `+0x24`
by `0x00A20F4E`; the byte-string operation and fallback contract below
qualifies that returned value.

Later calls to `0x00A20100` and `0x00A20070` read `u32(C_init+0x10)`
and `u32(C_init+0x0C)`, respectively. The base initializer explicitly
stores those returned values at M `+0x28` and `+0x34`, in that order,
after the forwarded call. Its direct object stores use M as their base. These
instructions consume the supplied context fields rather than constructing
an authenticated context-to-Q relation. They do not establish context
memory preservation across `0x00A21680` or qualify pointer aliasing.
The checked field reads have no context/state null guards. The application
context in the receiver chain below is supplied separately.

### Initializer operation result

`0x00A21680` first calls `0x00A21610` with the supplied receiver L
and key word. The latter uses `L+8` as receiver for the published
[byte-string lookup](scb-clip-factory-composition.md#class-key-and-registry-selection)
at `0x00A1FC00`. Its two explicit arguments are a local output-pair
address and the address of the key-word stack slot. Lookup writes an
owner/node pair to that output and interprets the first dword at its second
argument as a NUL-terminated byte-string pointer. Thus the raw key word is
used as the query pointer, rather than as an integer key or pointer-identity
test. Valid terminated query text and valid tree storage remain conditions.
When M `+0x18` retains the supplied
[class-key entry](cut-scheduler-clip-layout.md#registration), that query
word is the entry's class-name pointer.

On a normal valid-owner result with node N different from the lookup end
node, `0x00A21610` reads `V = u32(N+0x10)`. A nonzero V produces return
`V+8`; the ordinary end-node and zero-payload paths return zero. The owner
and sentinel checks have diagnostic calls to `0x009D22B4`, whose behavior
and state effects are unqualified. They do not prove a safe return for
invalid owner/node storage.

If the first result is nonzero, `0x00A21680` returns it directly.
Otherwise, after that call, it reads `F = u32(L+4)` and returns `F+8`
when F is nonzero, or zero when F is zero. The fallback is therefore used
for a zero first result, including the normal null-payload path, not only
for an absent key. Both results are dword address adjustments. These bodies'
own instructions do not dereference the adjusted return or authenticate its type.
The published base store puts this result at M `+0x24`.

The actual object supplied as L, its fallback field, the node payload type
and the later consumer of M `+0x24` remain unjoined. Use of the same lookup
helper and `+8` adjustment does not identify L with the separate SCB factory
registry object, or establish inequality between their values. This return
contract does not construct the material context-to-Q binding or qualify
all callee and alias state effects.

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

## Retained tx channel producer

The initializer's first channel store is conditional on a parsed runtime
container, rather than directly on serialized `Attr` bytes. At
`0x0083532B`, `0x00A2D1B0` receives the literal `frc`, a supplied
buffer/interface and descriptor-derived size. On its nonnull allocation
path, that helper calls `0x00A31680` and returns its result O. This
constructor installs table `0x01099454`; original RTTI identifies
`AnimationResourceNode` (COL `0x011870F0`, TypeDescriptor `0x012EE7A8`).
It explicitly clears O `+0x54` at `0x00A316C8`. Later calls receive
the resource and its callback interface at O `+0x48`, including the call
at `0x00A317C5` through the retained buffer/interface's virtual `+0x2C`.
Those callees' effects and completion are unqualified here. The constructor
has no later explicit O `+0x54` store; this is not a claim that the field
remains zero across those calls.

At `0x00835358..0x0083535D`, the initializer reads O `+0x54` and
stores it at M `+0x38`. The following selection requires that value T
to be a valid retained container. A diagnostic call on a zero value does
not establish safe continuation. For a T retaining the published
`MotionCurveContainer` table `0x01098B7C` (COL `0x01186B04`,
TypeDescriptor `0x012EE168`), its original virtual `+0x28` cell at
`0x01098BA4` selects `0x00A2B720`. The initializer calls that slot
with `(0,Attr)` at `0x00835393`, then supplies its return and clip
ordinal zero to `0x00A2B6F0` at `0x0083539B`.

For clip zero, let `H = u32(T+0x0C)`, `P = u32(H)` and
`N = u32(H+4)`. With stable valid storage and `0 < N <= 0xFFFF`,
`0x00A2B720` visits entries `P+8*j` in order. Helper `0x00A2D930`
reads `u32(u32(P+8*j))`, the first dword of the entry's source V.
Helper `0x00A2D470`, with receiver T `+0x10`, masks that index by
`0x7FFFFFFF` and returns:

```text
u32(T+0x10) + u32(u32(T+0x2C) + 4*(u32(V) & 0x7FFFFFFF))
```

The selector compares the resulting terminated bytes with `Attr` exactly
and case-sensitively. Its low-u16 return is the first matching j, or
`0xFFFF` after an ordinary miss. It does not check clip zero against a
clip count. `0x00A2B6F0` separately requires its unsigned clip ordinal
to be below byte T `+0x3C`, then compares the low-u16 group index with
the selected clip's signed dword count. Its accepted route returns
`G = u32(P+8*j+4)`; rejected indices return zero. The following
property selector assumes G is valid, so a missing group is not promoted
to a safe null-channel path.

At `0x008353A0..0x008353B0`, original selector cell `0x01097E24`
supplies pointer `0x01096320`, whose terminated text is `tx`.
The initializer calls `0x00810FE0` with G and that pointer, then stores
its return K at M `+0x68`. The selector visits the property pointers
at `u32(G)+4*i` for `i < u8(G+5)`. It calls each property's virtual
`+4` name getter and compares its terminated return bytes exactly with
the query. The first byte-equal match returns that property's pointer;
an ordinary miss returns zero. Valid names, unchanged arrays/counts and
preservation of the live lookup state across those virtual calls remain
conditions. The original selector cell and text must also retain their
values for this to be the `tx` query.

The published [parsed-group construction](mtb-joint-translation-write.md#parsed-curve-group-and-descriptor-transfer)
stores each constructed curve pointer in G's array and appends `(V,G)`
to the container's clip-zero table. If the same selected group and matching
property were produced by its type-2 route, K is the same successfully
constructed [FixedCurve](mtb-fixed-curve.md#type-2-key-record-and-runtime-layout),
with its supplied name pointer at K `+4`. Original table cell
`0x01098C8C` selects the published
[receiver +4 getter](timeline-weather-record-view.md#construction-and-pointer-attachment)
at `0x008D5570` for the name query, while cell `0x01098C94` selects
scalar evaluator `0x00A320B0` for virtual `+0x0C`. Under those same
object, table, name and storage conditions, retaining K at M `+0x68`
therefore joins the first material sampler call to the published FixedCurve
runtime construction and restricted evaluator contract. No parser or
evaluator body is needed again for that conditional join.

The selectors themselves construct no curve and impose no FixedCurve
type test. The actual O `+0x54` population, selected group's property
type and identity, selected scalar payload and context-to-Q binding remain
unjoined. The nearby serialized `Attr` text supplies none of those runtime
identities. An absent `tx` can store zero at M `+0x68`; the published
application prefix has no channel-pointer null guard before sampling.

## Update state and sample input

Application receives M in ECX and a separately supplied context `C_app`
as its first stack argument. At `0x0083548E..0x00835491`, it calls base
update `0x00A20F90` with those arguments. It then checks M `+0x51`
to decide whether to call the active-container helper; it does not test the
base update's return or the container helper's return before sampling.
Neither this pointer flow nor matching context offsets identifies `C_app`
with `C_init` or constructs the context-to-Q relation.

The base update first compares M `+0x1C` with 1 and explicitly clears
M `+0x2C`. For state 1, it stores 2 at M `+0x1C` and skips the delta
calculation. Otherwise, it obtains `R_app = u32(C_app+0x0C)` through
the published `0x00A20070` getter. Forwarder `0x00A1B830` adds
`0xD0` to that receiver and tail-jumps to `0x00A203B0`. The latter
returns the float at `R_app+0xE4` when byte `R_app+0xE8` is nonzero,
or x87 value 1 otherwise. The update stores this factor as a double.

Provider `0x00A1FFC0` reads `u32(C_app+0x0C)` and tail-jumps through
`0x00A1B840` to `0x00A20470`. That getter returns the dword at mutable
global `0x012ED240`; it does not read a further receiver field. The
[celestial aggregate](sky-celestial-layout-census.md) also uses this
provider. The [light-clip contract](weather-light-curve-container-shape.md#light-clip-colour-and-intensity-v)
identifies a writer and its frame-rate formula. This does not bind that
writer's invocation, the applicable units or the actual material context
on this path.

For the update's provider result, `CVTSI2SS` interprets EAX as signed
i32 and converts it to float32. `CVTSS2SD` promotes that rounded value
to double, `MULSD` multiplies by the stored factor, and `CVTTSD2SI`
truncates the double result to signed i32 on its finite, in-range path.
The resulting dword is stored at M `+0x2C`. At the join, the update adds
the current dwords at M `+0x28` and `+0x2C`, with 32-bit wrapping,
and explicitly stores the sum at M `+0x28` only when the unsigned sum
is below `0x7FFFFFFF`. This is a guarded store, without a saturation
store on the other branch. These instructions do not establish runtime
floating-point control state or qualify conversion exceptions and aliases.

The application then calls `0x00A1FFC0` independently at `0x008354A6`
and converts that EAX to float32 as signed i32. This is a separate read
of the mutable global from the update's call. Let A be
`u32(M+0x54)-4` when that field is nonzero, or zero otherwise. If
`u32(D+0x1C)` is nonzero, the denominator is replaced by the float at
`0x00FA5124`, whose original PE bytes encode `10000.0`. If `D+0x1C`
is zero, the code reads `u32(A+0x150)` and makes the same replacement
when that word is 0 or 1; other words retain the converted provider
result. There is no null guard before that A-field read. The selected
serialized D has `D+0x1C=0`, but the actual A and mode word remain
conditions. The constant supplies no seconds or calendar interpretation.

The primary-vtable cell at `0x01039F9C+0x24 = 0x01039FC0` selects
`0x00A210A0`, which returns `u32(M+0x28)`. This virtual byte offset
is separate from data field M `+0x24`; no consumer of that initializer
result is established here. At `0x008354F3..0x00835513`, application
loads the numerator with signed-i32 `FILD`, divides it by the float32
denominator with x87 `FDIV`, and stores the ratio as float32. The first
`FSTP` after the argument-space push aliases the former denominator
cell. The other three calls load that same stored ratio, rather than
dividing again. No finite, nonzero-denominator or bounded-ratio guarantee
follows from these instructions.

Thus, with valid channel pointers, expected argument cleanup and valid
unchanged local storage across the calls, each retained receiver at
M `+0x68/+0x6C/+0x70/+0x74` receives the same float32 ratio through
its virtual offset `+0x0C`, at `0x00835513/29/3F/55`. This prefix has
no channel-pointer null checks. The later container/count guards govern
parameter application, not these sampler calls. The input contract does
not identify the sampler implementations, consumed curve payload, returned
channel values or visible material effect.

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

The material accessor's actual context-to-Q binding remains unjoined.
[Base initialization](#initializer-context) consumes a supplied context.
Its [operation result](#initializer-operation-result) still requires the
actual L instance, valid lookup/query storage and an identified payload
consumer. The separate
[SCB factory path](scb-clip-factory-composition.md#parsed-clip-key-table)
joins P to Q for its fireworks scheduler construction; that separate path
does not identify this material context. The selected directory and
indirection bytes do not establish that the material accessor reaches this
constructed Q, the downstream payload parser, or FCurve scalar consumption.
No serialized Attr values are assigned.

The [retained tx channel](#retained-tx-channel-producer) has a conditional
parsed-group and FixedCurve consumer join, but its actual populated
container and selected property type remain unjoined. Registry lookup
remains conditional.
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
| Base clip initialization | `0x00A20F30` | `0x00A20F84` |
| Context/state operation forwarding | `0x00A20110` | `0x00A2011E` |
| Context +0x10 getter | `0x00A20100` | `0x00A20104` |
| Context +0x0C getter | `0x00A20070` | `0x00A20074` |
| Initializer result and fallback | `0x00A21680` | `0x00A216A5` |
| String-key payload result | `0x00A21610` | `0x00A21677` |
| Lookup output-pair ABI | `0x00A1FC00` | `0x00A1FC85` |
| Base update state | `0x00A20F90` | `0x00A20FF7` |
| Application provider forwarding | `0x00A1FFC0` | `0x00A1FFC8` |
| Update factor forwarding | `0x00A1B830` | `0x00A1B83B` |
| Provider thunk | `0x00A1B840` | `0x00A1B845` |
| Update factor getter | `0x00A203B0` | `0x00A203BD` |
| Mutable provider getter | `0x00A20470` | `0x00A20476` |
| Sample numerator getter | `0x00A210A0` | `0x00A210A4` |
| Group pointer selection | `0x00A2B6F0` | `0x00A2B71A` |
| Property-name selection | `0x00810FE0` | `0x00811049` |
| Resource allocation forwarding | `0x00A2D1B0` | `0x00A2D278` |
| Animation resource construction | `0x00A31680` | `0x00A317E0` |
| Group-name selection | `0x00A2B720` | `0x00A2B79F` |
| Group-name index read | `0x00A2D930` | `0x00A2D935` |
| Indexed byte-string pointer | `0x00A2D470` | `0x00A2D484` |

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

For initializer context flow, check only the material prefix
`[0x00835250,0x00835291)` in addition to the four listed base/forwarding
ranges. The prefix establishes the `0x28`-byte stack setup, incoming
context argument, base-call receiver and descriptor-call receiver. Reuse
the descriptor accessor and SCB view-producer contracts.
For the operation result, reuse the SCB byte-string lookup semantics and
check its output-pair/key-pointer ABI over the actual listed instruction
union `[0x00A1FC00,0x00A1FC85)`, totaling 133 bytes. The last instruction
is the three-byte `RET 8` at `0x00A1FC82`. The two listed operation
ranges establish the payload adjustment, post-call fallback read and
return branches. No additional material caller range is needed for its
published M `+0x24` store.

For update state and sampler inputs, reuse the material application's
precise prefix `[0x00835470,0x00835557)`, totaling 231 bytes. The
instruction-start filter ending at `0x008354B0` includes the instruction
at `0x008354AF` through exclusive end `0x008354B2`; the sample-input
filter `[0x008354AB,0x00835557)` ends exactly after the fourth call.
Check the listed seven update/provider/getter ranges, original vtable cell
`0x01039FC0` and float constant `0x00FA5124`. Reuse the published
context getter contract. Track ESP at `0x00835507..0x00835510` to
verify replacement of the denominator cell by the stored ratio and its
reuse by the following three calls. No sampler body is needed for this
argument contract.

For the first channel producer, reuse only initializer operand unions
`[0x00835291,0x00835349)` and `[0x00835358,0x008353CC)`, totaling
300 bytes. A start filter ending `0x00835345` includes the five-byte
instruction at `0x00835344` through exclusive end `0x00835349`.
Check the seven listed selection/resource ranges and the cited original
RTTI, table cells, selector pointer and terminated literals. Reuse the
published parsed-group pointer append, FixedCurve construction, name getter
and scalar slot contracts without reopening those bodies. This reproduces
the conditional first-channel join without reading a selected curve payload.

These selected bodies are contiguous. The exporter's `# size` generally
sums function-body AddressSet ranges; it does not promise an enclosing
span without gaps. The named input and native checks establish static
data and conditional own-body behavior, with no client execution.
