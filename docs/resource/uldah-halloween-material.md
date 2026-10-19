# Ul'dah interior hwa1 material dispatch boundary

Ul'dah interior `hwa1` has a `LayRaptureMaterial2Clip` on its named hide
side. Original PE RTTI and vtable cells establish that this class inherits
the published `RaptureMaterial2Clip` initializer and application methods.
The selected record contains ASCII `modulateColor` at the offset used by the
base constructor's string helper, conditional on retention of that record
pointer. Its selected serialized FCurve values and active material effect
remain unjoined.

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
record thus supplies the `modulateColor` input text if that runtime pointer
is D. The uninspected helper's input/output string relationship remains a
condition on the final generated material parameter names.

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

Accessor `0x00A20090` reads its context `+0x0C`, forwards the supplied
word to `0x00A1CA20` at `0x00A20098`, then calls `0x00A220A0` with
that result as ECX at `0x00A2009F`. Those uninspected helpers are the
missing D+0x14-to-descriptor edge. Zero does not independently prove that
the visible descriptor is selected. No serialized Attr values are assigned.

The intermediate constructor `0x00835950`, string helper `0x0064E6F0`,
FCurve construction helpers, registry lookup and active-container helper
`0x00834A80` remain conditions. Base construction captures current global
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
| Lay factory | `0x00835B90` | `0x00835C18` |
| Descriptor accessor | `0x00A20090` | `0x00A200A7` |
| Generic D retention | `0x00A21260` | `0x00A212A8` |

These selected bodies are contiguous. The exporter's `# size` generally
sums function-body AddressSet ranges; it does not promise an enclosing
span without gaps. The named input and native checks establish static
data and conditional own-body behavior, with no client execution.
