# Bitmap font scalar advance consumers

On the selected BitmapFont draw path, input dwords `0x20` and `0x1D`
both request scalar advancement using lookup key `0x20`. The branch adds
the returned integer to the horizontal pen and bypasses its glyph draw
dispatch. The scalar consumer supports successful original lookup and
successful conditional fallback. Its second lookup result is unchecked.
These are bounded static observations, not an encoding, complete layout,
default font, or live rendering contract.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Independent decoder | Python `3.14.7`, PE32 section mapping, Capstone `5.0.7`, x86 32-bit |
| Stored program | Project `ffxiv-1.23b`, program `ffxivgame.exe`, language `x86:LE:32:default` |
| Stored-program instrument | ghidra-cli `0.2.2`, Ghidra `12.1.4`, JDK `26.0.2.1` |

The original executable was read without execution or modification. Its
size, hash, PE machine `0x014C`, optional-header magic `0x010B`, and image
base matched. Each bounded span mapped into exactly one PE section's raw
file extent. Capstone decoded those original-file instructions. The existing
Ghidra program's stored executable hash, image base, name, and language
matched. Eleven bounded instruction and data spans had equal SHA-256
values in the original mapping and stored program memory. The scalar and
selected control branch instructions were inspected in both decoder outputs.
This was original-file static analysis with existing-program verification.
It was not a fresh import, retail-input CI run, runtime font check, or live
rendering test.

All locators are VA. Subtract `0x00400000` for RVA. Observations have
confirmed confidence within their stated bounds. `FUN_` identifiers are
address placeholders and imply no recovered source-level signatures.

The [text-key finding](bitmap-font-text-key-consumers.md#selected-construction-and-dispatch)
owns the selected construction and RTTI joins. The
[glyph finding](bitmap-font-glyph-consumers.md#lookup-and-normalized-output)
owns the numeric lookup and normalized output. The
[metric finding](bitmap-font-metric-consumers.md#stored-scale-ratio)
owns the stored scale ratio. This page joins those dependencies to the
selected scalar branch and its arithmetic order.

`B` denotes the complete BitmapFont pointer. `F` denotes the pointer held
at `B+0x228`, and `Q` the float32 value at `B+0x224`. The draw receiver
is `B+4`. `S` and `T` denote the draw's explicit float32 arguments at
entry-relative stack offsets `0x18` and `0x1C`. Its flag pointer is at
entry-relative `0x14`. These letters describe observed roles, not a complete
object layout or semantic argument names.

## Selected branch and arguments

Primary table VA `0x0110C194` contains `FUN_00CAC2E0` at byte offset
`0x04` and `FUN_00CAC1A0` at `0x40`. Draw table VA `0x0110C168`
contains `FUN_00CAC330` at `0x0C`. These selected entries were checked
against the original file. They do not establish immutable tables or all callers.

`FUN_00CAC330` derives complete `B` from its `B+4` receiver at
VA `0x00CAC33D`. It calls primary offset `0x04` with argument 1 at
VA `0x00CAC340` through `0x00CAC344`. A false low-byte result returns
dword `0xFFFFFFFF` at VA `0x00CAC346` through `0x00CAC352`. The
text loop is reached only after this passing check.

The comparisons at VA `0x00CAC680` through `0x00CAC688` direct either
current dword `0x20` or `0x1D` to VA `0x00CAC69F`. That branch calls
selected primary offset `0x40`, targeting `FUN_00CAC1A0`, with complete
receiver `B` and these explicit arguments:

| Order | Supplied value | Scalar entry-relative offset | Locator |
|---|---|---|---|
| First | Dword `0x20` | `0x04` | VA `0x00CAC6BB` |
| Second | Dword zero | `0x08` | VA `0x00CAC6B9` |
| Third | Incoming draw flag pointer | `0x0C` | VA `0x00CAC376`, `0x00CAC6B8` |
| Fourth | `S` in a float32 stack slot | `0x10` | VA `0x00CAC6B1` through `0x00CAC6B5` |
| Fifth | `T` in a float32 stack slot | `0x14` | VA `0x00CAC69F` through `0x00CAC6AB` |

The float arguments pass through x87 loads and float32 stores. This does
not establish surrounding x87 control state or exceptional-value preservation.
The complete scalar function does not read its second explicit argument.
Its meaning remains unresolved.

The branch bypasses the lookup, rectangle placement, and draw-dispatch block
at VA `0x00CAC49E` through `0x00CAC5F8`. It adds the scalar return to
the running 32-bit integer horizontal pen at VA `0x00CAC6BF`, then advances
to the following dword at VA `0x00CAC6C1` through `0x00CAC6CC`.
Interpretation: these two reached numeric values cause advancement without
this branch's glyph dispatch. The `0x1D` to `0x20` mapping supplies no
markup, encoding, or general whitespace identity.

## Lookup success and unchecked retry

`FUN_00CAC1A0` retains its third explicit argument as the flag pointer at
VA `0x00CAC1A5`. Its first call to `FUN_00CA6490` at VA `0x00CAC1BC`
uses receiver `F`. It supplies the first explicit key, that flag pointer,
and a local normalized-output pointer, in that order. Stack movement in
VA `0x00CAC1A9` through `0x00CAC1BC` establishes this argument join.

Lookup first calls `FUN_00CA6340` with receiver `F` and argument zero
at VA `0x00CA6491` through `0x00CA6495`. A false low-byte result returns
`0xFFFFFFFF` before any output store at VA `0x00CA649A` through
`0x00CA64A2`. A passing result permits the low-16-bit key read at
VA `0x00CA64A9`.

The numeric lookup uses the key's high byte to select a byte from `F+0x20`.
Byte `0xFF` fails. The zero-extended page byte multiplied by 256 plus
the key's low byte selects a word from `F+0x24`. Word `0xFFFF` fails.
Otherwise the zero-extended word index multiplied by 14 selects a record
from `F+0x28`. These observations are at VA `0x00CA64A5` through
`0x00CA64F2`. Sentinel failures return `0xFFFFFFFF` without output
stores at VA `0x00CA64DA` through `0x00CA64DF`. Successful normalization
returns zero at VA `0x00CA6591` through `0x00CA6594` or
`0x00CA65B0` through `0x00CA65B4`.

The scalar tests its first result as signed at VA `0x00CAC1C1` through
`0x00CAC1C3`. A nonnegative result proceeds to consumption. A negative
result retries key `0x3013`, reloading its receiver from `B+0x228` and
reusing the flag and local output pointers at VA `0x00CAC1C5` through
`0x00CAC1D6`. It next reads
normalized byte `+0x06` at VA `0x00CAC1DB` without testing the second
result.

The supported success cases are original key `0x20` lookup success, or
first failure followed by key `0x3013` lookup success. The first failure
can be a failed readiness result as well as a sentinel lookup result.
Fallback selection is conditional, and fallback success is not guaranteed.
The scalar does not initialize this local output before the calls. Both
observed lookup failure returns leave it unwritten. If both fail, the scalar
still reads the local byte. No defined advance, safe failure result, or
deterministic retained glyph is established for that case.

## Normalized advance dependencies

The successful lookup first copies record byte `+0x06` to normalized
byte `+0x06` at VA `0x00CA651D` through `0x00CA6521`. Its caller flag
tests read the supplied pointer, not a flag field from the selected record.

| Condition | Effect before scalar consumption | Locator |
|---|---|---|
| `F+0x18` nonzero and low flag byte contains `0x02` | Add copied byte `+0x08` to normalized byte `+0x06` using eight-bit arithmetic, retaining the low eight bits. | VA `0x00CA652B`, `0x00CA6536`, `0x00CA6554` through `0x00CA655C` |
| `F+0x18` zero | Bypass that byte addition. | VA `0x00CA6534`, `0x00CA6561` through `0x00CA6575` |
| Low flag byte contains `0x08` | Clear normalized byte `+0x02` and replace normalized byte `+0x06` after the earlier copy or addition. | VA `0x00CA6579` through `0x00CA6588` |
| `0x08` override, key at least `0x0100` | Store the low eight bits of dword `F+0x2C` as byte `+0x06`. | VA `0x00CA658A` through `0x00CA658E` |

The key comparison is 16-bit and unsigned. With the `0x08` override,
direct key `0x20` takes the low-key operations at VA `0x00CA6597`
through `0x00CA65AD`. Successful fallback key `0x3013` takes the
high-key store. The low-key operations are:

1. Form three times the dword pattern at `F+0x2C` with 32-bit arithmetic,
   retaining the low 32 bits.
2. Interpret that pattern as signed 32-bit and multiply by signed
   `0x66666667`, retaining a signed 64-bit product.
3. Retain its upper 32-bit half and arithmetic-right-shift by one.
4. Obtain the shifted half's sign bit by a logical right shift of 31, then
   add that zero-or-one term to the shifted half with 32-bit arithmetic.
5. Store the low eight result bits as normalized byte `+0x06`.

Neither override clamps or saturates. The scalar subsequently sign-extends
the stored byte. No unsigned advance, semantic flag name, legal combination,
or default flag value follows.

## Scalar widths and pen relation

The arithmetic in `FUN_00CAC1A0` consumes normalized byte `+0x06` as
signed 8-bit. The widths below describe observed SSE conversions and
arithmetic, not recovered source-language declarations.

| Order | Operation | Locator |
|---|---|---|
| 1 | Sign-extend the byte to signed 32-bit, then convert directly to float64. | VA `0x00CAC1DB`, `0x00CAC1F1` |
| 2 | Load `Q` as float32, promote to float64, and multiply by that signed advance in float64. | VA `0x00CAC1E0`, `0x00CAC1EE`, `0x00CAC1F5` |
| 3 | Load `S` as float32, promote to float64, and multiply the existing product in float64. | VA `0x00CAC1F9` through `0x00CAC202` |
| 4 | Load `T` as float32, promote to float64, and multiply the existing product in float64. | VA `0x00CAC206`, `0x00CAC20C`, `0x00CAC215` |
| 5 | Reduce the product to float32. | VA `0x00CAC219` |
| 6 | Begin an integer additive term at zero. Set it to one only if signed dword `F+0x18` is at most 3 and low-byte flag bit `0x02` or `0x01` is set. | VA `0x00CAC20F`, `0x00CAC211`, `0x00CAC21D` through `0x00CAC229` |
| 7 | Convert the term to float32. Promote it and the reduced product separately to float64, then add in float64. | VA `0x00CAC22E` through `0x00CAC23A` |
| 8 | Reduce the sum to float32, truncate toward zero to signed 32-bit, and return that integer. | VA `0x00CAC23F` through `0x00CAC24B` |
| 9 | Add the returned integer to the caller's horizontal pen with 32-bit integer arithmetic, retaining the low 32 bits. | VA `0x00CAC6BF` |

The term in order 6 is separate from lookup's `0x02` byte addition and
`0x08` replacement. Its flag access reads a dword at VA `0x00CAC21F`,
although only low-byte bits are tested. Its threshold is signed.

Interpretation: truncation precedes the integer pen update. Preserve the
multiplication order and both float32 reductions. The additive term follows
the first reduction. This function applies no half-unit offset or absolute
value and establishes no nearest-rounding or nonnegative-advance rule.

This branch writes no drawing rectangle and does not add to a floating
current-x accumulator. For a later ordinary glyph that reaches placement,
the current integer pen converts first to float32 and then to float64 at
VA `0x00CAC529` through `0x00CAC52D`. Signed normalized x-offset byte
`+0x02` converts directly to float64 and is multiplied by the earlier
horizontal float32 scale promoted to float64 at VA `0x00CAC4FA`,
`0x00CAC505`, and `0x00CAC51B` through `0x00CAC525`. The offset
product is added to the promoted pen at VA `0x00CAC531`. The result is
reduced to float32 at VA `0x00CAC539` and stored at VA `0x00CAC54E`.
The [metric finding](bitmap-font-metric-consumers.md#draw-scaling-and-placement)
owns that earlier scale calculation. This join establishes no global origin
or pixel placement convention.

## Readiness and unsupported cases

The selected initial draw check reaches `FUN_00CAC2E0`. It forwards
argument 1 to `FUN_00CA6340` at VA `0x00CAC2E0` through `0x00CAC2EE`.
A false result performs no `Q` store. A true result converts signed dwords
`B+0x220` and `F+0x2C` separately to float32, promotes both to float64,
divides the first by the second, reduces to float32, and stores `Q` at
VA `0x00CAC2FD` through `0x00CAC31A`. Its false return is at
VA `0x00CAC328` through `0x00CAC32B`. There is no explicit denominator-zero
or scalar-range check there.

The scalar itself does not refresh `Q`. Direct scalar invocation therefore
depends on already held object state. A failed refresh does not clear that
state. The selected admitted draw path has the prior passing refresh, but
each lookup still requires its own passing readiness check. Readiness
internals and external effects are outside this joined contract.

The scalar reloads `B+0x228` again at VA `0x00CAC1E8` before the
`F+0x18` threshold. Object fields are read at their observed stages. No
coherent snapshot across readiness, lookup, normalization, and arithmetic
is established.

The usable success cases require the selected table relationships and valid
reached pointers and storage. They do not establish resource availability,
table or record bounds, upstream argument defaults, allowed scalar domains,
pointer alias restrictions, or legal flag combinations. The supplied flag
pointer must support accesses that execute, including the scalar's conditional
dword read. The text loop reads dwords until zero. A complete wrapper-admission
contract is not established here.

Signed-integer truncation interpretations apply only to representable finite
results. No defined policy is supplied for NaN, infinity, out-of-range
conversion, denominator zero, invalid pointers, malformed or changing records,
altered tables, or overflow as a usable layout result. SSE widths do not pin
MXCSR, and the x87 argument transfers do not pin x87 control state.
Successful fallback remains conditional. Both-failed lookup remains
unsupported.

No encoding identity, character repertoire, external text occurrence, marker
grammar, font selection, alignment, newline or tab layout, general line
layout, resource binding, raster precision, or full appearance is established.

## Reproduction bounds

Verify the input identity before inspecting the original mapped instructions
and data. Account for stack movement and receiver adjustment. Raw listings,
retail bytes, and analyzed program state remain outside the public tree.

| Purpose | Bounded VA span |
|---|---|
| Draw, selected branch, and later pen placement | `0x00CAC330` plus `0x3D0` bytes |
| Complete scalar consumer | `0x00CAC1A0` plus `0xB0` bytes |
| Complete lookup and normalization | `0x00CA6490` plus `0x128` bytes |
| Readiness call and result context | `0x00CA6340` plus `0x150` bytes |
| Selected scale refresh | `0x00CAC2E0` plus `0x4E` bytes |
| Selected primary entries | `0x0110C194` plus `0x50` bytes |
| Selected draw entries | `0x0110C168` plus `0x2C` bytes |

The eleven-span identity comparison additionally covered auxiliary context
`0x00CAC080` plus `0x6B` bytes and existing metric constants
`0x00F59898` plus 24 bytes, `0x00F57E48` plus 8 bytes, and
`0x00F62F80` plus 4 bytes. Inclusion in that comparison does not establish
a new claim about every entry or byte in those spans.
