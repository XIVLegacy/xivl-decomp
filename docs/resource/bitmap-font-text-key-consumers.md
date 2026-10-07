# Bitmap font text bytes and glyph keys

On the selected static BitmapFont paths, a reached ordinary-text callback
can preserve each graphic byte `0x21` through `0x7E` as the same unsigned
number through byte copying, dword conversion, and the 16-bit glyph lookup
argument. This finding establishes the bounded handoff. It does not identify
an external text occurrence, an encoding, a universal text layout, or a live
rendering result.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Independent instrument | Python `3.14.7`, PE32 section mapping, Capstone `5.0.7`, x86 32-bit |

A new independent instrument read the supplied original file without copying,
executing, or modifying it. It asserted the size, SHA-256, PE signature,
machine `0x014C`, optional-header magic `0x010B`, and image base before
inspection. Each requested VA span mapped into exactly one section's raw file
extent. Capstone decoded the mapped instructions. No stored Ghidra analysis
or previous extractor body supplied the proof. This is original-file static
analysis, not fresh-import, retail-input CI, or live-client validation.

All addresses are VA. Subtract `0x00400000` for RVA. Observations have
confirmed confidence within the cited bounds. `FUN_` names are address
placeholders. The notation below does not imply recovered source-level names,
signatures, or a complete object layout.

`B` denotes the complete BitmapFont pointer. `D=B+0x10` denotes its embedded
processor receiver. `W` denotes the byte wrapper passed on the selected path.
`U` denotes the wrapper whose elements are dwords, with count at `U+0x14`
and storage discriminator at `U+0x18`. `F` is the FontData pointer loaded
from `B+0x228`. The existing
[glyph consumer finding](bitmap-font-glyph-consumers.md) owns the glyph-record
lookup and normalized output. The
[metric](bitmap-font-metric-consumers.md),
[runtime bridge](bitmap-font-runtime-bridge-consumers.md), and
[shader](bitmap-font-shader-consumers.md) findings retain their separate
geometry, setup, and downstream program boundaries.

## Selected construction and dispatch

Observation: `FUN_00CAD030` calls `FUN_00CC0580` with complete `B` at
`0x00CAD033`. The base construction supplies `B` as the explicit argument
to `FUN_00CC14B0` with receiver `D` at `0x00CC05D4` through
`0x00CC05DF`. That constructor retains the argument at `D+0x54` at
`0x00CC150B`.

The BitmapFont constructor installs primary table `0x0110C194` at `B`,
draw table `0x0110C168` at `B+4`, and processor table `0x0110C0BC`
at `D`, at `0x00CAD04E`, `0x00CAD054`, and `0x00CAD062`.
Their preceding locator pointers identify `0x0119CC3C`, `0x0119CD9C`,
and `0x0119CDC4`, with respective complete-object offsets 0, 4, and
`0x10`. Each names TypeDescriptor `0x0130BD4C`, whose name at
`0x0130BD54` is `.?AVBitmapFont@Font@Component@@`.

| Selected table | Byte offset | Target |
|---|---|---|
| Primary `B` | `0x14` | `FUN_00CC0400` |
| Primary `B` | `0x18` | `FUN_00CBFF10` |
| Primary `B` | `0x1C` | `FUN_00CBFEA0` |
| Primary `B` | `0x28` | `FUN_00CC0220` |
| Primary `B` | `0x2C`, `0x30` | `FUN_00CBFE20`, `FUN_00CBFE30` |
| Primary `B` | `0x40` | `FUN_00CAC1A0` |
| Draw `B+4` | `0x08`, `0x0C` | `FUN_00CACEB0`, `FUN_00CAC330` |
| Processor `D` | `0x10` | `FUN_00CC0C40` |

These are selected construction and table relationships. They do not prove
immutable tables, a live receiver, or an exhaustive caller inventory.

## Byte copying and callback arguments

Observation: `FUN_00CC1C50` receives a byte pointer, callback pointer, and
receiver adjustment at entry-relative stack offsets 4, 8, and `0x0C`.
It retains its receiver `D` and current byte pointer. The ordinary branch
at `0x00CC1E3E` through `0x00CC1E5D` calls `FUN_00CB24E0` with
five explicit arguments, in order: current byte pointer, wrapper `D+0xBC`,
local consumed-byte output, delimiter pointer `D+0x58`, and 1.

`FUN_00CB24E0` clears the destination through `FUN_00445530` and calls
`FUN_00CB23D0` for the current byte. For a byte above `0x0D`, the latter
compares the unsigned current byte with successive sign-extended bytes of
the supplied NUL-terminated delimiter sequence at `0x00CB2471` through
`0x00CB24B2`. No match returns 0 and consumed count 1. A match returns
1 and consumed count 1. The caller's fifth argument is 1, so both results
reach the same append branch at `0x00CB2517` through `0x00CB2527`.
This observation requires readable delimiter state. It establishes no
delimiter default or legal pointer domain.

For consumed count 1, `0x00CB2530` through `0x00CB2541` zero-extends
the current byte, appends it through `FUN_00447950`, increments the
consumed total by 1, and advances the byte pointer by 1. The append helper
stores the low byte of its argument and writes a trailing NUL at
`0x00447962` through `0x0044796E`. Its capacity helper invalidates the
cached character count at `0x00447013` through `0x0044701E` and updates
the byte length on its available-capacity branch at `0x0044709E`.
Allocation and reporting effects remain conditions.

The processor compares the wrapper's character count plus `D+0xB4` with
`D+0x64` at `0x00CC1E62` through `0x00CC1E72`. On the below-limit
branch, it supplies wrapper `D+0xBC` to the callback and uses receiver
`D` plus the incoming adjustment at `0x00CC1E78` through `0x00CC1E83`.
The limit branch invokes a substring helper before its callback at
`0x00CC1F38` through `0x00CC1F84`. The unchanged-wrapper claim here is
limited to the below-limit branch.

A nonnegative ordinary callback result permits current-pointer advancement
by the helper's consumed-byte total at `0x00CC1E94` through
`0x00CC1EC3`. A negative result instead calls `FUN_00CC1B90` and returns
the negative result at `0x00CC1FB3` through `0x00CC1FD0`. This pointer
movement is separate from the numeric decoder's advancement below.

## Dword conversion and numeric identity

Observation: `FUN_004492F0` supplies the byte pointer read from `W+0`
and destination `U` to `FUN_00449210` at `0x00449332` through
`0x00449336`, after capacity work. The conversion resets the prior
destination range through `FUN_0044A540`, `FUN_0044A020`, and
`FUN_00449CE0`. The last helper updates `U+0x14` and the dword terminator
at `0x00449D4E` through `0x00449D62` on its reached removal path.

For each non-NUL source pointer, `FUN_00449210` passes the pointer and a
local dword-output pointer to `FUN_00445850` at `0x00449236` through
`0x0044923C`. It advances the source pointer by the helper's returned
integer at `0x00449247`. On a first byte below `0x80`, the decoder
zero-extends that byte to its dword output and returns advance 1. Those
operations are at `0x00445850` through `0x00445867`, followed by the
optional output store at `0x0044597E` through `0x00445989`.

The converter groups outputs into chunks of `0x100` dwords and a final
partial chunk, adds a dword zero terminator, and counts only through the
first zero in each chunk before calling `FUN_0044A800` at
`0x00449251` through `0x0044927D` and `0x00449290` through
`0x004492BC`. On the selected nonalias append path, that helper copies
the dwords unchanged through `FUN_00449A40`, updates `U+0x14`, and
stores a dword zero terminator at `0x0044A8C3` through `0x0044A8E8`.
The copy helper's passing-capacity branch is at `0x00449A56` through
`0x00449A75`. This describes successful wrapper operations, not guaranteed
allocation or reporting behavior.

Interpretation: a copied run containing only bytes `0x21` through `0x7E`
contains no numeric zero, and each source step forms its identical unsigned
dword value while advancing by one byte. The byte-count helper
`FUN_00445E50` likewise advances by one per such byte and records the count
at `0x00445E7E` through `0x00445E87` and `0x00445EBC` through
`0x00445ED3`, after count invalidation.

The complete decoder span `0x00445850` through `0x0044598A`, end
exclusive, also has multibyte numeric branches. This finding assigns no
encoding name to them and does not extend the unchanged-run claim to them.
That helper has no remaining-length, continuation-pattern, overlong,
scalar-domain, or surrogate-domain check. The conversion's zero-delimited
chunk counting is another reason not to infer a complete malformed-input
contract from the decoder alone.

## Selected measurement handoff

Observation: primary wrappers `FUN_00CBFE20` and `FUN_00CBFE30` add
`0x10` to `B` and transfer to setups `FUN_00CC1FE0` and
`FUN_00CC2190`. The setups obtain the input wrapper's `+0` pointer through
`FUN_00445210` and call the processor with zero receiver adjustment and
callbacks `FUN_00CC1400` or `FUN_00CC0A40`, at `0x00CC20DB` through
`0x00CC20F0` or `0x00CC2255` through `0x00CC2276`.

`FUN_00CC1400` supplies `W`, pointer `D+0x28`, and the float arguments
from `D+0x2C` and `D+0x98` to complete-`B` virtual offset `0x1C` at
`0x00CC1408` through `0x00CC1428`. The selected target is
`FUN_00CBFEA0`. Callback `FUN_00CC0A40` makes the same initial
measurement call at `0x00CC0AC9` after its height-state check.

`FUN_00CBFEA0` supplies `W`, its character count, the flag pointer, and
the two float arguments to primary virtual offset `0x14` at
`0x00CBFEC1` through `0x00CBFED0`. Selected `FUN_00CC0400` converts
`W` to local `U` through `FUN_004492F0`, then supplies `U`, the passed
count, flag pointer, and floats to primary virtual offset `0x18` at
`0x00CC0440` through `0x00CC0472`.

Selected `FUN_00CBFF10` bounds the passed count by `U+0x14`, reads
dword elements at `0x00CBFF87`, and passes a graphic-byte value unchanged
as the first explicit argument of primary virtual offset `0x40` at
`0x00CBFFF5` through `0x00CBFFFF`. Its remaining explicit arguments are
the next dword value, the flag pointer, and the two floats. Selected
`FUN_00CAC1A0` forwards the first argument and flag pointer to lookup
`FUN_00CA6490` using receiver `F` at `0x00CAC1A9` through
`0x00CAC1BC`.

The lookup reads only the low 16 bits of its first explicit argument at
`0x00CA64A9`. The supplied flag and normalized-output pointers are its
second and third explicit arguments. Entry-relative stack offsets are 4,
8, and `0x0C`, including the intervening pushes and the callee-cleaned
argument of the initial readiness call. Thus every admitted graphic-byte
number is the same 16-bit lookup key.

## Selected draw handoff

Observation: reached setup `FUN_00CC2320` passes callback
`FUN_00CC1B50`, zero adjustment, and the wrapper's byte pointer to the
processor at `0x00CC23DA` through `0x00CC23ED`. No upstream call into
this setup is established here. The callback converts nonempty `W` into
`U=D+0x164` through `FUN_004492F0` and passes `U` to
`FUN_00CC1780` at `0x00CC1B68` through `0x00CC1B7A`.

That helper asks complete-`B` virtual offset `0x28` for a fit result at
`0x00CC17B8` through `0x00CC17EA`. The selected table target is
`FUN_00CC0220`. At `0x00CC17EC` through `0x00CC17F7`, a positive
result is retained and a nonpositive result is replaced with 1. When that
retained-or-replaced value equals `U+0x14`, the branch at `0x00CC17F7`
through `0x00CC1844` calls draw virtual offset `0x0C` with receiver
`B+4` and `U` as its second explicit argument. The other arguments, in
order, are a local position pointer, `D+0x78`, `D+0x88`, `D+0x28`,
the two floats from `D+0x2C` and `D+0x98`, and zero. Other branches
split, recurse, and change processor state. They establish no universal
wrapping policy in this finding.

Selected `FUN_00CAC330` first calls complete-`B` virtual offset 4 and
exits on false at `0x00CAC336` through `0x00CAC352`. On the admitted
draw path, it selects inline or indirect dword storage according to
`U+0x18`, reads the current dword at `0x00CAC464` through `0x00CAC486`,
and advances by four bytes at `0x00CAC6C1` through `0x00CAC6D0`.
The graphic-byte values reach the lookup call unchanged at
`0x00CAC49E` through `0x00CAC4AB`, with receiver `F` loaded from `B+0x228`, caller
flag pointer, and local normalized output. Lookup's low-word argument read
therefore preserves the identical key on this selected draw path as well.
Successful lookup reaches the published draw dispatcher at `0x00CAC5F8`.
No dispatch, binding, or raster success is established.

Separately, draw virtual offset `0x08` selects `FUN_00CACEB0`. It converts
its second explicit byte-wrapper argument through `FUN_004492F0` at
`0x00CACF11` through `0x00CACF1A` and forwards the converted wrapper
as second argument to the same draw virtual offset `0x0C` at
`0x00CACF4B` through `0x00CACF56`. This is another selected conversion
entry, not proof that the processor called it.

## Control, replacement, and failure boundaries

The admitted graphic-byte subset excludes NUL, CR, LF, tab, space, marker
bytes, and `0x7F`. The following branches keep those boundaries explicit.

| Path | Bounded observation | Locator |
|---|---|---|
| Processor termination | NUL stops the input loop. | `0x00CC1D45`, `0x00CC1EC6` |
| Processor marker | Byte `0x02` enters a separate callback-table path except when the following byte is `0x1D` or `0x1F`. | `0x00CC1D50` through `0x00CC1D6F` |
| Processor newline | CRLF and isolated CR or LF dispatch through processor virtual offset `0x10`, selecting `FUN_00CC0C40`. | `0x00CC1DFF` through `0x00CC1E1E`, `0x00CC1E36` through `0x00CC1E3C`, `0x00CC1EB3` |
| Byte helper marker | Marker subcodes `0x1D` and `0x1F` supply output bytes `0x1D` and `0x1E`, respectively, and consumed count 4. | `0x00CB2407` through `0x00CB2438`, tables `0x00CB24B4`, `0x00CB24C4` |
| Measurement control | Value `0x7F` skips. Tab has another advance branch. `0x1D` supplies `0x20` and `0x1E` supplies `0x2D` to the scalar consumer. | `0x00CC0021` through `0x00CC0076` |
| Measurement replacement | A dword value at least `0x10000` supplies key `0x3013` to the scalar consumer. | `0x00CBFFB5` through `0x00CBFFDF` |
| Scalar lookup failure | A negative first lookup result retries key `0x3013`. It does not test the second result before reading normalized advance. Successful fallback and defined output after both failures are not established. | `0x00CAC1C1` through `0x00CAC1DB` |
| Draw control | `0x1E` supplies `0x2D`. Tab has another advance branch. Space and `0x1D` use a scalar advance path. | `0x00CAC490` through `0x00CAC499`, `0x00CAC66B` through `0x00CAC688`, `0x00CAC69F` through `0x00CAC6BF` |
| Draw replacement | A value at least `0x10000` directly selects key `0x3013`. | `0x00CAC68A` through `0x00CAC69A`, joined to `0x00CAC4B9` through `0x00CAC4C5` |
| Draw lookup failure | A negative first result retries key `0x3013`. A negative second result skips this element's draw and advance and continues to the next dword. | `0x00CAC4B0` through `0x00CAC4CC`, `0x00CAC6C1` through `0x00CAC6D0` |

No control or flag semantic names are assigned. The original external text
source, markup grammar, encoding identity, complete malformed-marker behavior,
all allocation and helper effects, legal argument domains, caller defaults,
all callers, complete font layout, resource identity, runtime state, and live
appearance remain unresolved. These static joins do not establish Unicode,
UTF-8, UTF-16, Shift-JIS, or any full character repertoire contract.

## Reproduction bounds

Verify the input identity first. Inspect mapped original instructions and
data, accounting for stack movement and subobject adjustment. Ranges below
have exclusive ends. Keep raw listings and retail bytes outside the public tree.

| Purpose | Bounds |
|---|---|
| Construction joins | `0x00CAD030` through `0x00CAD080`, `0x00CC0580` through `0x00CC05FE`, `0x00CC14B0` through `0x00CC151D` |
| Tables, locators, and name | `0x0110C0B8` plus `0x1C`, `0x0110C164` plus `0x2C`, `0x0110C190` plus `0x50`, the three locators above each plus `0x14`, `0x0130BD54` plus `0x1F` |
| Processor and byte classifier | `0x00CC1C50` through `0x00CC1FE0`, `0x00CB23D0` through `0x00CB24B2`, data `0x00CB24B4` plus `0x20`, `0x00CB24E0` through `0x00CB2560` |
| Byte wrapper helpers | `0x00445530` through `0x00445548`, `0x00447950` through `0x00447980`, `0x00447010` through `0x004470A9`, `0x00445E50` through `0x00445ED4`, `0x00445210` through `0x00445220` |
| Setup and callbacks | `0x00CBFE20` through `0x00CBFE40`, `0x00CC1FE0` through `0x00CC210D`, `0x00CC2190` through `0x00CC229C`, `0x00CC2320` through `0x00CC242D`, `0x00CC1400` through `0x00CC14A2`, `0x00CC0A40` through `0x00CC0C40` |
| Numeric decoder and conversion | `0x00445850` through `0x0044598A`, `0x00449210` through `0x004492CD`, `0x004492F0` through `0x00449346` |
| Dword wrapper helpers | `0x0044A540` through `0x0044A5E0`, `0x0044A020` through `0x0044A0F0`, `0x00449CE0` through `0x00449DBF`, `0x0044A800` through `0x0044A8F8`, `0x00449A40` through `0x00449A76` |
| Measurement and lookup | `0x00CBFEA0` through `0x00CBFEE0`, `0x00CC0400` through `0x00CC04C0`, `0x00CBFF10` through `0x00CC0080`, `0x00CAC1A0` through `0x00CAC250`, `0x00CA6490` through `0x00CA65B8` |
| Draw handoff | `0x00CC1B50` through `0x00CC1B85`, `0x00CC1780` through `0x00CC1964`, `0x00CC0220` through `0x00CC0400`, `0x00CACEB0` through `0x00CACF70`, `0x00CAC330` through `0x00CAC700` |
