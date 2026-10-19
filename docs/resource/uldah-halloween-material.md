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
The intermediate-constructor, descriptor-selection and view-construction
observations below use Python 3.12.0 with those same libraries to decode
original PE instructions.
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
construction branch. The uninspected helper's input/output string
relationship remains a condition on the final generated material parameter
names.

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

## Exact remaining edge

The selected directory includes an `@FCURVE` entry at `0x7C630` with
descriptor text `FCurve0000.fcr` at `0x7C660`. The adjacent word at
descriptor `+0x10` is `0x107`; the candidate span
`[0x7C680,0x7C787)` fits the entry, begins `SEDBmtb`, and contains
literal `Attr` at `0x7C782`. These facts do not join that span to native
selection or give its scalar words consumed channel meanings.

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
It calls uninspected `0x009D5475` with `(candidate,key,8)` and returns
`candidate+0x10` on the first zero result, or zero if the loop finishes.
The precise remaining construction boundary is the producer of those
directory fields, the actual Q-to-P storage edge, and the indirection
word at T+2. The selected SCB entry and visible `FCurve0000.fcr` text
alone do not establish these joins. Construction-path calls to uninspected
`0x00A26D70` at `0x00A2500D` and `0x00A26DE0` at `0x00A2507B`
are the retained directory-construction discriminators. No serialized Attr
values are assigned.

String helper `0x0064E6F0`, FCurve scalar construction helpers, registry
lookup and active-container helper `0x00834A80` remain conditions.
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

For the D-retention edge, check the full intermediate-constructor range and
reuse the prior Lay/base forwarding and generic-store observations. The
precise supporting snippets are `[0x008350D0,0x00835109)`,
`[0x00834970,0x008349A9)`, `[0x00A21260,0x00A21278)` and
`[0x00A212A5,0x00A212A8)`. These include the forwarding prologues,
D store and generic stack cleanup.

These selected bodies are contiguous. The exporter's `# size` generally
sums function-body AddressSet ranges; it does not promise an enclosing
span without gaps. The named input and native checks establish static
data and conditional own-body behavior, with no client execution.
