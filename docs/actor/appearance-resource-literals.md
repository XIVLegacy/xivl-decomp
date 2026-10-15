# Appearance resource literals

The retail 1.23b `ffxivgame.exe` contains the literals `f%03d/fac`,
`h%03d/hir`, and `/client/chara/%s%03d/kao/%s%s%04d`. Bounded instruction
observations associate those addresses with local calls near `0x006b7f80`.
They establish literal text, callsite relationships, and local machine scalar
operands pushed immediately before two literal pointers. They do not establish
the meaning of the substituted values.

## Source and method

The executable identity, retail build `2012.09.19.0001`, and section layout are
recorded in [PE layout](../resource/pe-layout.md#identity-and-version-resources).
All addresses here are VAs at image base `0x00400000`.

The instruction observations used LLVM21 `llvm-objdump`, instrument SHA-256
`47ceb4e3477a2dd569ca032dd9f5c3af05ccc01e7ac302de2727b362ba49c94f`.
Contiguous known instructions and their encodings were checked against
independent bounded file slices mapped through the PE header. Relative call
targets and caller ESP arithmetic were independently checked. Helper semantic
spans stop at the first return. Literal reads were independent, unbuffered,
and one byte at a time, stopping immediately at the first NUL.

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
`0x006b805a` to `0x009d4f83`. Bounded observations of `0x00630660` and
`0x006306a0` establish the stores and local joins below. They do not establish
a C signature or semantic role for either target. The target `0x009d4f83`
remains uninspected.

## Machine scalar and literal operands

For each call below, `x` is the machine word read from the listed source
operand, and `u32(x)` is its unsigned 32-bit value. Stack words are numbered
from the first word after the return address. Each `S` is the caller ESP
before the first argument push at the stated address.

| Helper target | Source operand | Definition of S | Stack words after return address |
|---|---|---|---|
| `0x00630660` | `ESI+0x68` | ESP at `0x006b7f9a` | 1: `x`, 2: pointer `S+0x18`, 3: pointer `S+0x28`, 4: pointer `S+0x14`, 5: pointer `S+0x1c` |
| `0x006306a0` | `ESI+0x6c` | ESP at `0x006b7fdc` | 1: `x`, 2: pointer `S+0x18`, 3: pointer `S+0x14`, 4: pointer `S+0x1c` |

The observed stores through those pointer words are:

| Helper target | Pointer word | Actual destination | Stored value |
|---|---:|---|---|
| `0x00630660` | 2 | `S+0x18` | `u32(x) >> 30` |
| `0x00630660` | 3 | `S+0x28` | `(u32(x) >> 14) & 0x7fff` |
| `0x00630660` | 4 | `S+0x14` | `(u32(x) >> 8) & 0x3f` |
| `0x00630660` | 5 | `S+0x1c` | `u32(x) & 0xff` |
| `0x006306a0` | 2 | `S+0x18` | `u32(x) >> 30` |
| `0x006306a0` | 3 | `S+0x14` | `(u32(x) >> 10) & 0x3ff` |
| `0x006306a0` | 4 | `S+0x1c` | `u32(x) & 0x3ff` |

Each store is four bytes. The actual destinations within each of these calls
are distinct. This is a property of these caller destinations, not a
nonaliasing contract for arbitrary invocations.

The semantic spans for `0x00630660` are `[0x00630660,0x00630692)` and
`[0x00630692,0x0063069a)`. Its plain RET at `0x00630699` has no immediate
argument cleanup, and the observed path has no earlier stack-pointer
modification. After return, ESP is `S-0x14`. The reload at `0x006b7fb7`
reads `[ESP+0x28]`, which is `[S+0x14]`. The scalar push at `0x006b7fbb`
immediately precedes the literal pointer push at `0x006b7fbc` of
`0x00fd2d94`, before the call at `0x006b7fce` to `0x009d4f83`.

The semantic span for `0x006306a0` is `[0x006306a0,0x006306cc)`. Its plain
RET at `0x006306cb` has no immediate argument cleanup, and the observed path
has no earlier stack-pointer modification. After return, ESP is `S-0x10`.
The reload at `0x006b7ff0` reads `[ESP+0x24]`, which is `[S+0x14]`. The
scalar push at `0x006b7ff4` immediately precedes the literal pointer push at
`0x006b7ff5` of `0x00fd2da0`, before the call at `0x006b8007` to
`0x009d4f83`.

## Interpretation and limits

The text identifies resource-path literals in the local instruction neighborhood. The
[native item-appearance boundary](item-appearance-boundary.md) records the
broader appearance chain. The observations here do not establish field names,
packed-bit meanings, format-argument meanings, helper or callee semantic roles,
a face or hair selection contract, concrete resource IDs or models, head or
body attachment, or model-to-joint transforms. They also do not establish
supported or unsupported cases, animation, successful resource creation, or
live rendering parity.

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

For the helper stores and returns, use the same independent PE-mapped slice
and contiguous-encoding checks with these bounded windows. The window end is
exclusive. A decoder guard is a raw decoding bound, not a semantic extent.

| Read window | Decoder guard stop | Semantic stop |
|---|---|---|
| `[0x00630660,0x006306a0)` | `0x00630692` | Prefix exhausted. Continue with the next window. |
| `[0x00630692,0x006306b2)` | `0x006306a4` | First plain return ends at `0x0063069a`. Later raw guard runs remain uninterpreted. |
| `[0x006306a0,0x006306e0)` | `0x006306d2` | First plain return ends at `0x006306cc`. Later raw guard runs through `0x006306d4` remain uninterpreted. |

The first window alone is insufficient to establish the first helper's return.
Join its contiguous continuation before checking the caller ESP arithmetic,
actual four-byte destinations, reloads, and scalar and literal pointer pushes.
Stop semantic interpretation at each first plain return. These observations
do not establish whole-function extents or general calling conventions.
