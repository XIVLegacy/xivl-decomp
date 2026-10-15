# Appearance resource literals

The retail 1.23b `ffxivgame.exe` contains the literals `f%03d/fac`,
`h%03d/hir`, and `/client/chara/%s%03d/kao/%s%s%04d`. Bounded instruction
observations associate those addresses with local calls near `0x006b7f80`.
They establish literal text and callsite relationships, without establishing
the meaning of the substituted values.

## Source and method

The executable identity, retail build `2012.09.19.0001`, and section layout are
recorded in [PE layout](../resource/pe-layout.md#identity-and-version-resources).
All addresses here are VAs at image base `0x00400000`.

The instruction observations used LLVM21 `llvm-objdump`, instrument SHA-256
`47ceb4e3477a2dd569ca032dd9f5c3af05ccc01e7ac302de2727b362ba49c94f`.
Contiguous known instructions and their encodings were checked against
independent bounded file slices mapped through the PE header. Literal reads
were independent, unbuffered, and one byte at a time, stopping immediately at
the first NUL.

## Literal observations

| Literal VA | Text | Read length including first NUL |
|---|---|---:|
| `0x00fd2d94` | `f%03d/fac` | 10 |
| `0x00fd2da0` | `h%03d/hir` | 10 |
| `0x00fd2dac` | `/client/chara/%s%03d/kao/%s%s%04d` | 34 |

## Local instruction observations

| Source operand | Callsite and target | Subsequent literal pointer | Following callsite and target |
|---|---|---|---|
| `ESI+0x68` | `0x006b7fb2` -> `0x00630660` | `0x00fd2d94` | `0x006b7fce` -> `0x009d4f83` |
| `ESI+0x6c` | `0x006b7feb` -> `0x006306a0` | `0x00fd2da0` | `0x006b8007` -> `0x009d4f83` |

The common literal pointer `0x00fd2dac` is associated with the call at
`0x006b805a` to `0x009d4f83`. These observations do not inspect any of the
call targets or establish their effects, identities, or calling conventions.

## Interpretation and limits

The text identifies resource-path literals in the local instruction neighborhood. The
[native item-appearance boundary](item-appearance-boundary.md) records the
broader appearance chain. The observations here do not establish field names,
packed-bit meanings, format-argument meanings, a face or hair selection
contract, concrete resource IDs or models, head or body attachment, or
model-to-joint transforms. They also do not establish unsupported-case
behavior, successful resource creation, or live rendering parity.

## Reproduction

Supply the executable identified by the linked PE record and verify its size
and SHA-256 before reading it. Map each literal VA through its PE section and
repeat the first-NUL reads above without reading neighboring literals.

For the local callsite relationships, independently read the code slice from
`0x006b7f21` to exclusive end `0x006b8121`. Decode with the identified
`llvm-objdump` and stop at the guarded decoder boundary `0x006b8113`.
Require contiguous known instructions and agreement between their encodings
and the independently read slice. Check the source operands, literal
pointers, and relative call targets at the cited sites. This bounded recipe
does not establish the complete extent of a function or the meaning of a
callee.
