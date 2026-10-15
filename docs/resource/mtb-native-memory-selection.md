# Bounded MTB native memory selection

The native setup, type predicate and lookup at VAs `0x00A2D5E0`,
`0x00A2D430` and `0x00A2D510` establish a conditional memory selection
ending at a candidate address plus `0x10`. They do not establish complete
MTB file framing or playback semantics.

## Identity and method

The target is retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The published [PE identity](pe-layout.md#identity-and-version-resources)
owns the executable identity. All locators below are VA with image base
`0x00400000`.

The producing instrument is LLVM `llvm-objdump` `21.0.0`, executable size
`45236288` bytes, SHA-256
`47ceb4e3477a2dd569ca032dd9f5c3af05ccc01e7ac302de2727b362ba49c94f`.
The observation uses bounded x86-32 disassembly. Encoded instruction bytes
were independently checked against the identified executable. Complete
native and system dependency closure for the instrument remains unknown.

To reproduce, verify the target and instrument identities, disassemble only
the following VA windows in Intel syntax, and compare the encoded
instruction bytes with the executable through its PE section mapping.
The stop VA is exclusive. Raw-byte verification is bounded separately.

| Purpose | Start VA | Stop VA | Raw-byte verification cap |
|---|---|---|---:|
| Consumer | `0x00A30910` | `0x00A313E0` | `2768` |
| Setup | `0x00A2D5E0` | `0x00A2D6D1` | `256` |
| Type predicate | `0x00A2D430` | `0x00A2D4A1` | `128` |
| Lookup | `0x00A2D510` | `0x00A2D5D1` | `208` |

Do not extend these windows to infer the uninspected helper or the fallback
return. No resource payload or pointed-to selector bytes are part of this
observation.

## Supplied memory and setup

Let M be the setup object and B its supplied memory pointer. `LE16` and
`LE32` denote little-endian unsigned memory reads. Address arithmetic is
native 32-bit, modulo `2^32`. At `0x00A2D5E0..0x00A2D601`, setup stores:

| Object position | Stored value |
|---|---|
| `M+0x00` | `B` |
| `M+0x04` | `B` |
| `M+0x08` | `H = B + LE16(B+0x0E)` |
| `M+0x0C` | `H+4` |

At setup, the field later read as U explicitly contains `H+4`.
B is supplied memory.
Its correspondence to an actual file or outer container base is unproved.

At `0x00A2D689..0x00A2D6A5`, setup passes zero and pointer value
`0x01098C38` to lookup `0x00A2D510`. It stores the result at `M+0x20`
and the result plus `0x10` at `M+0x24`. The bytes at `0x01098C38`,
their access width and their selector meaning remain unknown.

## Exact four-byte predicate

At `0x00A2D430..0x00A2D469`, the predicate reads B from `M+0x04`.
It returns `AL=1` exactly when bytes `B+4..7` equal either
`4D 54 42 20` or `6D 74 62 00`, and `AL=0` otherwise on these paths.
Bytes `B+0..3` and memory bounds are not established.

## Conditional lookup relation

Lookup calls uninspected helper `0x00A2D750` at `0x00A2D51A`.
Pointer forwarding and a signed result D are visible. The helper's exact
calling contract, selector meaning, purity and effects on M remain unknown.

After that helper, `0x00A2D533..0x00A2D556` reads T from `M+0x08`,
N as `LE32(T)`, X from `M+0x00`, and U from `M+0x0C`.
When signed `D >= 0` and unsigned `N > 0`, the loop reads
`r_i = LE32(U+4*i)` and forms `S_i = X+r_i` modulo `2^32`.
The loop proceeds while unsigned `i < N`, with the continuation visible
at `0x00A2D5BA..0x00A2D5C2`.

The immediate return at `0x00A2D597..0x00A2D59F` requires every one of
these conditions, observed in `0x00A2D523..0x00A2D595`:

- Signed `D >= 0` and unsigned `N > 0`.
- An in-loop candidate with `LE32(S_i) = D`.
- Bytes `X+4..7` equal `4D 54 42 20` or `6D 74 62 00`.
- Unsigned raw `LE32(X+8) >= 0x42`.
- The counter at `stack+0x18` equals the comparison value at `stack+0x1C`.

Under those conditions, the visible return is `P = S_i+0x10` modulo
`2^32`. Setup stores P at `M+0x20` at `0x00A2D697`.
The threshold `0x42` has no established semantic version meaning.
The stack comparison value is not an established parameter across the
unknown helper contract.

Only if the object fields retain their setup values across that helper
may the relation be rewritten as:

```text
X = B
T = H
U = H+4
N = LE32(H)
S_i = B + LE32(H+4+4*i)
P = S_i+0x10
```

The unchanged-fields condition and all selection conditions above are
required. Actual setup-to-lookup field retention and selection of an actual
resource row remain unproved.

## Consumer join and unresolved boundaries

Setup's `RET 8` and the caller's stack-address calculations at
`0x00A3093A..0x00A30954` join the accepted-predicate consumer load at
`0x00A30992`, `ESP+0x7C`, to `M+0x20`, where M is restored
`ESP+0x5C`. The consumer's unsigned 16-bit memory reads at `P+0x08`
and `P+0x0A` do not establish actual file fields.

The bounded fallback observation at `0x00A2D5CE` computes `S_i+4` in
EAX. The following return code is unobserved. No fallback return or
alternate framing contract follows.

Bounds checks, malformed-input handling, the actual file base, outer
framing, key selection, helper effects and complete lookup behavior remain
unresolved. These are static executable observations. They establish no
native execution, retail-resource acceptance, chosen resource row, runtime
acceptance, animation channel, timing or playback result.
