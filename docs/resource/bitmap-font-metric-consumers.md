# Bitmap font metric consumers

The cited retail client paths preserve FSIZ, FSMX, BSLN, and HMGN as raw
dwords and consume them through specific signed integer and floating-point
operations. They also derive a stored source-metric scale ratio. The
observations establish these bounded consumers, not a global line-height,
baseline coordinate, font selection, style, or rendering-precision contract.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Stored program | Project `ffxiv-1.23b`, program `ffxivgame.exe` |
| Stored language | `x86:LE:32:default` |
| Stored-program producer | ghidra-cli `0.2.2`, Ghidra `12.1.4`, JDK `26.0.2.1` |
| Independent file decoder | Python PE32 section mapping and Capstone, reported version `5.0.7` |

All addresses are VA. Subtract `0x00400000` for RVA. The file's size, SHA-256,
PE32 header, and image base matched this identity. A new read-only Ghidra
script checked the stored program's name, `getExecutableSHA256()`,
`getImageBase()`, and language. Every bounded instruction and data span below
had identical bytes in the independently mapped file and stored program.
The instruction and constant observations were independently inspected from
both instruments. This used the existing analyzed program. It was not a fresh
import, retail-input CI run, or live client rendering test.

The [Ghidra Program API](https://ghidra.re/ghidra_docs/api/ghidra/program/model/listing/Program.html)
defines the program identity and memory interfaces. The
[Capstone Python interface](https://www.capstone-engine.org/lang_python.html)
defines the independent decoder interface. These are tool definitions, not
retail evidence. The
[Intel instruction reference](https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-2a-manual.pdf)
entries for CVTSI2SS, CVTSI2SD, CVTSS2SD, CVTSD2SS, CVTPD2PS, CVTTSS2SI,
and CVTTSD2SI establish the signed conversions, widths, and truncation used
in the interpretations below.

The existing [glyph consumer finding](bitmap-font-glyph-consumers.md) owns
the FontData and BitmapFont identity anchors and normalized glyph offsets.
Here `F` is the complete FontData-relative pointer, `C` is its callback
pointer at `F+8`, and `B` is the complete BitmapFont-relative pointer. The
draw interface is `B+4`. `R` denotes raw dword `B+0x220`, `Q` denotes
float32 `B+0x224`, and `B+0x228` holds the observed `F` pointer. These
letters are notation, not recovered member names.

Observations have confirmed confidence only within their stated instruction
and data ranges. Interpretations are marked separately. Placeholder function
names imply no recovered source-level signature.

## Raw metric stores

Observation: `FUN_00CA65C0` preserves its incoming callback pointer at VA
`0x00CA65FE`. Construction writes the callback vtable at `F+8` at VA
`0x00CA62E5`. The callback vtable at VA `0x0110B834` has complete-object
locator VA `0x0119C9DC` with offset 8. Its slot-0 thunk subtracts 8 at VA
`0x00CA6D90`. These observations establish the callback-to-complete-object
adjustment used here.

On each matching tag path, the reader loads a dword from the current chunk
pointer plus 8 and stores its raw 32-bit pattern without a byte swap, mask,
or numeric conversion:

| Tag | Comparison data VA | Source load VA | Store VA | Callback destination | Complete destination |
|---|---|---|---|---|---|
| FSIZ | `0x0110B418` | `0x00CA691A` | `0x00CA6921` | `C+0x24` | `F+0x2C` |
| FSMX | `0x0110B4A4` | `0x00CA6994` | `0x00CA699B` | `C+0x28` | `F+0x30` |
| BSLN | `0x0110B530` | `0x00CA6A10` | `0x00CA6A17` | `C+0x30` | `F+0x38` |
| HMGN | `0x0110B5BC` | `0x00CA6A8A` | `0x00CA6A91` | `C+0x2C` | `F+0x34` |

The four comparison operands contain the respective four ASCII tag
characters. The comparisons occur at VA `0x00CA68E8`, `0x00CA6962`,
`0x00CA69DE`, and `0x00CA6A58`. Mismatches call a reporting helper before
joining the store path if that helper returns. Termination on mismatch and
malformed-input behavior are not established.

The unsigned comparison and branch at VA `0x00CA6929` through `0x00CA692D`
skip the FSMX chunk path when `F+0x18` is less than 3. VA `0x00CA69A5`
through `0x00CA69A8` then copy retained FSIZ from `F+0x2C` to `F+0x30`.
Otherwise the FSMX chunk supplies `F+0x30`.

The unsigned comparison and branch at VA `0x00CA6A1F` through `0x00CA6A23`
skip the HMGN chunk when `F+0x18` is less than 2. That skip performs no
`F+0x34` store. Construction clears `F+0x2C`, `F+0x30`, `F+0x34`, and
`F+0x38` at VA `0x00CA630E`, `0x00CA6311`, `0x00CA6314`, and
`0x00CA6317`. This does not prove that a repeated load with a skipped chunk
resets an existing value.

Interpretation: these scalar payloads are retained in native little-endian
order by these paths. This is not a format-wide endian rule. Tag names alone
do not establish legal domains, units, or coordinate conventions.

## Stored scale ratio

Observation: `FUN_00CAD030` receives complete `B`. It copies its second
explicit stack argument into `B+0x220` at VA `0x00CAD038` and
`0x00CAD048`, and its first into `B+0x228` at VA `0x00CAD03C` and
`0x00CAD071`. It stores primary vtable VA `0x0110C194` at `B` at VA
`0x00CAD04E`, and draw vtable VA `0x0110C168` at `B+4` at VA
`0x00CAD054`. It loads float32 -1.0 from VA `0x00F62F80` at VA
`0x00CAD040` and initializes `Q` at VA `0x00CAD069`.

The primary vtable's locator pointer at VA `0x0110C190` identifies locator
VA `0x0119CC3C`, whose subobject offset is zero. Primary slot 1 at VA
`0x0110C198` points to `FUN_00CAC2E0`. Draw slot 1 points to thunk
`FUN_00CAD360`, which subtracts 4 before transferring to that function at
VA `0x00CAD363`.

`FUN_00CAC2E0` calls `FUN_00CA6340` with `F` at VA `0x00CAC2EE`.
A false result returns false without a `Q` store at VA `0x00CAC328`
through `0x00CAC32B`. On true, it:

1. Converts signed dword `R` to float32 at VA `0x00CAC2FD`.
2. Converts signed dword `F+0x2C` to float32 at VA `0x00CAC305`.
3. Promotes each float32 value to float64 at VA `0x00CAC30A` and
   `0x00CAC30E`.
4. Divides the first by the second at VA `0x00CAC312`.
5. Reduces the quotient to float32 at VA `0x00CAC316` and stores `Q` at
   VA `0x00CAC31A`, then returns true.

There is no explicit denominator-zero or scalar-range branch in this bounded
function. The two integer-to-float32 conversions precede division.

Interpretation: `Q` is a source-metric scale ratio used geometrically by the
cited glyph consumers. The constructor argument's upstream source, allowed
values, user-facing unit, and defaults remain unresolved. Initial -1.0 is a
construction value. Two `Q` writers were identified within this scope.
This is not an exhaustive writer inventory.

## BSLN-derived and composite scalar outputs

Observation: primary slot 3 at VA `0x0110C1A0` points to
`FUN_00CAC010`. It calls slot 1 at VA `0x00CAC01A` without branching
on the result. It reads `F+0x38` as signed 32-bit and converts to float32
at VA `0x00CAC022`. It promotes that value and `Q` to float64 at VA
`0x00CAC02F` and `0x00CAC033`, multiplies them at VA `0x00CAC036`,
adds float64 0.5 from VA `0x00F59898` at VA `0x00CAC03A`, and truncates
toward zero to signed 32-bit at VA `0x00CAC042`. There is no observed clamp,
absolute value, or readiness-success check in this getter.

Interpretation: this is a BSLN-derived scaled integer output. Adding 0.5
before truncation is not a general nearest-rounding rule for negative inputs.
It does not establish a baseline coordinate convention.

Observation: primary slot 17 at VA `0x0110C1D8` points to
`FUN_00CAC080`. It returns zero unless signed dword `F+0x18` is less
than 4 and the low byte at its flag-pointer argument contains bit `0x02`.
The signed comparison and flag tests are at VA `0x00CAC086` through
`0x00CAC093`. On the admitted path it:

1. Converts signed `R` to float32 at VA `0x00CAC095`, then promotes
   it and `Q` to float64.
2. Multiplies successively by `Q`, the first explicit float32 argument,
   and the second explicit float32 argument at VA `0x00CAC0AC`,
   `0x00CAC0B9`, and `0x00CAC0C6`. Each argument is promoted to float64
   before use.
3. Adds float64 8.0, subtracts float64 1.0, and multiplies by float64
   0.125 at VA `0x00CAC0CA`, `0x00CAC0D2`, and `0x00CAC0DA`.
   The constants are at VA `0x00F57E48`, `0x00F598A0`, and
   `0x00F598A8`.
4. Truncates toward zero to signed 32-bit at VA `0x00CAC0E2`.

Call this result `A(flags, S, T)`. No style or margin meaning is assigned.
Primary slot 18 at VA `0x0110C1DC` points to `FUN_00CAC0F0`.
The wrapper sets local flags to 2 at VA `0x00CAC107` and calls slot 17 at
VA `0x00CAC113` with its incoming float32 argument and float32 1.0.
It preserves the integer result.

Primary slot 2 at VA `0x0110C19C` points to `FUN_00CABF90`.
It calls slot 1 at VA `0x00CABF9B`. Both result branches obtain
`A(2, 1.0, 1.0)` through slot 18. On success:

- VA `0x00CABFA9` and `0x00CABFAD` read signed dwords `F+0x30` and
  `F+0x34`. Both are converted separately to float32 before promotion
  to float64 at VA `0x00CABFC5` through `0x00CABFDD`.
- VA `0x00CABFD0` multiplies the first by promoted `Q`.
- VA `0x00CABFE5` adds the second, without multiplying it by `Q`.
- VA `0x00CABFD9` and `0x00CABFE1` convert `A` from signed integer
  through float32 to float64. VA `0x00CABFEA` adds this to the composite.
- VA `0x00CABFEE` truncates the result toward zero to signed 32-bit.

On failure, VA `0x00CABFF6` reads `F+0x34`, and VA `0x00CAC004`
and `0x00CAC00A` add `R` and that retained dword to `A` with 32-bit
integer additions. Only the low 32 result bits are retained.

Interpretation: `F+0x30` supplies the scaled FSMX-or-FSIZ term, and
`F+0x34` supplies a separate unscaled HMGN term in this composite getter.
These are consumed roles, not a semantic expansion of the tag names. Direct
xref queries for the two getters returned only their primary vtable data
slots. Virtual callers remain possible. No bounded caller join establishes
newline stepping, line height, or baseline placement from these getters.

## Draw scaling and placement

Observation: `FUN_00CAC330` receives `B+4` and derives complete `B`
at VA `0x00CAC33D`. It calls primary slot 1 with argument 1 at VA
`0x00CAC344`, exiting on failure. This path recomputes `Q` before glyph
placement. Let `S` and `T` be its explicit float32 arguments at entry-relative
stack offsets `0x18` and `0x1C`. The flag pointer is at entry-relative
`0x14`, and the position pointer is at entry-relative 4.

At VA `0x00CAC355` through `0x00CAC38E`, promoted `Q` and `S` are
multiplied in float64. This product is reduced to float32 for vertical scale
`V`. The unreduced float64 product is multiplied by promoted `T` before
reduction to float32 for horizontal scale `H`. Thus `H` is not computed
from the already reduced `V`. The read at draw-interface-relative `+0x220`
is complete `B+0x224`, which holds `Q`.

VA `0x00CAC3AA` through `0x00CAC3DA` initialize the running horizontal
integer with `A(flags, S, T)` plus the position's float32 x component
truncated toward zero. VA `0x00CAC3D6` through `0x00CAC3EE` initialize
the stored vertical integer with `A(2, S, 1.0)` plus its float32 y component
truncated toward zero. Both additions are 32-bit integer additions.

The lookup output begins at the local pointer formed at VA `0x00CAC49E`.
The following placement observations use normalized offsets from that output:

| Output | Observed operations | Locators |
|---|---|---|
| Drawing x origin | Signed byte `+0x02` converted directly to float64, multiplied by promoted `H`, then added to the running horizontal integer converted through float32 to float64. Result reduced to float32. | VA `0x00CAC4FA`, `0x00CAC51B` through `0x00CAC539` |
| Drawing y origin | Signed byte `+0x03` converted directly to float64, multiplied by promoted `V`, then added to the stored vertical integer converted through float32 to float64. Result reduced to float32. | VA `0x00CAC50C`, `0x00CAC560` through `0x00CAC5A0` |
| Drawing width | Signed byte `+0x04` converted through float32 to float64, multiplied by promoted `H`, then reduced to float32. | VA `0x00CAC4DC`, `0x00CAC508`, `0x00CAC517`, `0x00CAC53D` through `0x00CAC544` |
| Drawing height | Signed byte `+0x05` converted through float32 to float64, multiplied by promoted `V`, then reduced to float32. | VA `0x00CAC4EB`, `0x00CAC535`, `0x00CAC576` through `0x00CAC587` |

The drawing rectangle is separate from the source rectangle, which retains
unscaled origin words and byte extents. No BSLN, FSMX, or HMGN field is
directly read in this placement block. Its auxiliary methods consume fields
as specified above. Flag `0x04` also affects another drawing argument through
VA `0x00CAC39A` through `0x00CAC3A4` and `0x00CAC5AD` through
`0x00CAC5B5`. These observations do not name that effect or cover all styles.

VA `0x00CAC5FD` through `0x00CAC667` consume signed normalized advance
byte `+0x06`. It is converted directly to float64, multiplied successively
by promoted `Q`, `S`, and `T`, and reduced to float32 at VA
`0x00CAC638`. A zero-or-one term is then added after promotion to float64.
That term is one only when signed `F+0x18` is at most 3 and the low flag
byte contains either bit `0x01` or `0x02`, as observed at VA
`0x00CAC630` through `0x00CAC64A`. The sum is reduced to float32 at
VA `0x00CAC65F`, truncated toward zero to signed 32-bit at VA
`0x00CAC663`, and added to the running horizontal integer at VA
`0x00CAC667`.

Interpretation: rectangle placement uses the earlier reduced `H`, while
advance recomputes its own multiplication sequence. These paths do not
establish bit-identical results for all inputs.

## Reproduction bounds and unresolved behavior

The raw observations remain outside the public tree. A reproducer needs the
identified executable and these bounded VA spans:

| Purpose | Instruction or data bounds |
|---|---|
| Complete FontData construction and callback basis | `0x00CA6290` plus `0x97` bytes, `0x00CA65C0` plus `0x90`, `0x00CA6D90` plus 8, data `0x0110B830` plus 12 and `0x0119C9DC` plus 20 |
| Metric chunk paths | `0x00CA68B5` plus `0x1EF` bytes, four tag data operands listed above, 4 bytes each |
| Complete BitmapFont construction and scale writer | `0x00CAD030` plus `0x50` bytes, `0x00CAD360` plus 8, `0x00CAC2E0` plus `0x4E`, data `0x00F62F80` plus 4 |
| Vtable and complete-object bases | Data `0x0110C168` plus `0x78` bytes, `0x0119CC3C` plus 20 and `0x0119CD9C` plus 20 |
| Scalar getters and auxiliary wrapper | `0x00CABF90` plus `0x81` bytes, `0x00CAC010` plus `0x3D`, `0x00CAC080` plus `0x6B`, `0x00CAC0F0` plus `0x2C` |
| Arithmetic constants | Data `0x00F59898` plus 24 bytes and `0x00F57E48` plus 8 |
| Draw setup and placement | `0x00CAC330` plus `0xB8` bytes, `0x00CAC3E3` plus `0x17`, `0x00CAC49E` plus `0x20D` |
| Bounded direct references | Xrefs to `0x00CABF90` and `0x00CAC010` |

Inspect decoded instructions rather than treating a decompiler's inferred
names or types as authority. For arithmetic, retain each conversion and
operation order. The float32 and float64 widths do not by themselves pin
MXCSR state. The auxiliary wrapper and draw setup also pass float32 values
through x87 loads and stores. Their surrounding x87 control state was not
resolved. Device transformations and final raster precision were not tested.

No denominator-domain, configured-size, source-font selection, global origin,
pixel-center, baseline, line-height, newline, or full style contract is
established. NaN, infinity, invalid integer conversions, overflow,
nondefault floating-point control state, malformed chunks, changed vtables,
and repeated-load behavior were not tested. The integer truncation
interpretations apply to representable finite results. Retain these gaps
instead of assigning guessed values or precision rules.
