# Sqwt DoubleAnimation Velocity

[Documentation index](../README.md) | [Clock and capture](sqwt-double-animation-clock.md)

The DoubleAnimation `Velocity` property replaces the Duration with a span
derived from the distance between its endpoints. This page records how setup
chooses between the two, how the span and the per-sample value follow, and
what the page leaves open. These are static mechanics. They do not establish
a rendered value, a live animation or the units of any target property.

## Identity and method

The input is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
base `0x00400000`. Addresses are VAs.

The method is focused ghidra-cli disassembly and decompilation of the cited
functions, plus memory reads of the cited constants and strings. Decompiler
renderings supplied leads. Every instruction-level claim below was read from
the disassembly. STATIC identifies a read from instructions or data.
INFERRED identifies a bounded consequence. UNKNOWN retains an open edge.

## The property

| Item | Value |
|---|---|
| Name string | `Velocity` at `0x0107DC14` |
| Property info record | `0x013621A4`, index 2 (the initializer at `0x00F298D0` stores 2 at `0x013621B8`) |
| Receiver slot | `+0x140`, a float, set up by the constructor `0x009B5440` at `0x009B54D5..0x009B54EE` |
| Default | the float sentinel `10000000.0`, bytes `80 96 18 4B` at `0x0107DAD0` |

The clock page calls the same `+0x140` value Speed. The two names refer to one
slot. STATIC.

## Setup

Setup `0x009B4680` compares `+0x140` with the sentinel at `0x009B496A`. It
derives the span and the rate in one of two ways. STATIC.

| Velocity | Span D (qword at `+0x190`) | Rate (float at `+0x188`) |
|---|---|---|
| Sentinel (unset) | the Duration qword at `+0x48`, in milliseconds | `abs(start - end) * 1000.0 / D` (`0x009B497A..0x009B49C7`) |
| Set, nonzero | `trunc(abs(start - end) * 1000.0 / Velocity)`, truncated toward zero | the Velocity value itself (`0x009B49C9..0x009B49F9`) |
| Set, equal to 0.0 | all ones (`OR EDI, 0xFFFFFFFF` at `0x009B48B5` is stored at `0x009B4A09` and `0x009B4A0F`) | the Velocity value, 0.0 |

The constant 1000.0 is the double at `0x00F91C48` (`0x408F400000000000`). The
zero compared against is the double at `0x00F62F70`. The conversion from the
x87 result to the qword calls `0x009D6636` at `0x009B49F4`. Start and end are
the values already chosen by the setup described on the clock page, with
their offsets applied (`+0x17C` and `+0x180`). A set Velocity therefore
overrides the derived span, and a Velocity of zero yields a span that no
elapsed input equals. STATIC.

The helper `0x009D6636` truncates toward zero. It runs FISTP into a qword
(`0x009D6645`) and FILD of that qword back (`0x009D6649`), so the stored
integer is the nearest one under the current x87 rounding mode. It then
subtracts the integer from the original (`0x009D6659`) and reads the sign of
the remainder as a float. For a positive original (`0x009D667D..0x009D6693`)
it adds `0x7FFFFFFF` to the remainder bits and subtracts the carry from the
qword, which lowers the integer by one when the remainder is negative, that
is when FISTP rounded up. For a negative original (`0x009D665F..0x009D667B`)
it flips the remainder sign first and adds the carry, which raises the
integer by one when FISTP rounded away from zero. A quotient of 666.67
therefore stores 666 and not 667, and -2.7 stores -2. The structure removes
any dependence on the rounding mode. STATIC for the instructions, INFERRED
for the mode independence.

Setup reaches `0x009B4A17` only on the Velocity-set path. There it passes a
pointer to a copy of D to `0x009AC730` (`0x009B4A36`), which writes D into the
base Duration property at record `+0x48` through descriptor `0x0136182C` and
`0x009C2130`. The unset path jumps from `0x009B49C7` to `0x009B4A3B` and skips
the write. A Velocity animation therefore leaves a Duration of D in the
property that the clock page names as the Duration qword. STATIC.

A negative Velocity gives a negative quotient. Truncation keeps the sign, so
the stored D is the two's-complement qword of a negative integer, for example
`0xFFFFFFFFFFFFFED4` for -300, and no elapsed input equals it. A quotient
between -1 and 0 truncates to 0. The rate stored at `+0x188` is the negative
Velocity itself. STATIC for the stores. INFERRED for the consequence that a
negative Velocity of ordinary magnitude never reaches its end check, as a
Velocity of zero does not.

The two acceleration type bytes (`+0x168` and `+0x16A`) set bit 0 of the flag
byte at `+0x198` when either selects a ramp, and setup clears the bit first at
`0x009B46B9`. The bit is independent of Velocity. STATIC.

## Evaluation

Evaluator `0x009B4A70` reads the elapsed qword at record `+8` and takes one of
three paths. STATIC.

- Elapsed equal to D (both dwords at `+0x190` and `+0x194`): the current
  value is the end value at `+0x180`.
- Flag bit 0 clear: the current value moves linearly from the start toward the
  end at the rate in `+0x188` per second. For start below end it is
  `start + elapsed * rate / 1000.0` and for end below start it is
  `start - elapsed * rate / 1000.0`. No clamp appears in this body, so the
  completion check above is what ends the run.
- Flag bit 0 set: the elapsed input is divided by D into a fraction, passed
  through the ramp helper `0x009B4530` with the two ramp pairs at `+0x16C` and
  `+0x174`, and the current value is `start + (end - start) * fraction`. In
  this path Velocity acts only through D.

The result is serialized through `0x00988D50` and written with the target
property setter `0x00912850`, as on the clock page.

Consequences. INFERRED from the paths above with a forward-moving elapsed
input.

- A Velocity animation from 0 to 300 at 100 has D of 3000 and writes a
  Duration of 3000 into `+0x48`, whatever Duration the markup also names. That
  the timeline's own completion test reads the `+0x48` Duration, and so
  completes the run after 3000 ms, is not traced. UNKNOWN. The evaluator's
  end value at Q equal to D is STATIC.
- A span with a fractional quotient drops the fraction. To-From of 2 at
  Velocity 3 gives 666, not 667.
- The span depends on the endpoints read at Begin, so a To or From bound to a
  property changes the span with the value at that moment.
- A Velocity of 0.0 holds the start value indefinitely, because the end check
  never matches and the linear path adds nothing.

## Open items

- UNKNOWN: the span when the endpoints are equal and Velocity is set. The
  quotient is zero there and the zero-span scheduler branch of the clock page
  is the only edge traced.
- UNKNOWN: whether the timeline completion test at `0x009AC820` reads the
  Duration at `+0x48` that the Velocity path writes, as the consequence above
  needs.
- UNKNOWN: the units of the bound Velocity of the cast gauge template
  (`FloatData.Value0` of the templated parent). The page establishes the
  arithmetic on a float and not what writes the slot.
- UNKNOWN: whether elapsed input is clamped to D before the evaluator for a
  Velocity-derived D, as the clock page shows for a Duration-derived one.

## Reproduction

Verify the executable identity above. Read the string at `0x0107DC14`, the
initializer at `0x00F298D0`, the constructor window `0x009B5440..0x009B5570`,
the constants at `0x0107DAD0`, `0x00F91C48` and `0x00F62F70`, setup
`0x009B4680..0x009B4A5B` and evaluator `0x009B4A70`. Check the branch at
`0x009B496A..0x009B4975` against the sentinel, the stores at
`0x009B49C9..0x009B4A17`, the call at `0x009B4A36` with `0x009AC730`, the
helper `0x009D6636..0x009D66AA` and the two evaluator paths.
