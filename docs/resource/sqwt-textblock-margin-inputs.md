# Sqwt TextBlock Margin inputs

The identified Party parameter carriers use an exact unqualified `Margin`
attribute on elements whose XML local name is `TextBlock`. Their values are
literal lists of four comma-separated, unitless decimal integer tokens.
The observations below describe input spelling and numeric categories.
No actual Margin value, Name value or XML body is reproduced.

## Inputs and instrument

The retail build is FINAL FANTASY XIV 1.23b `2012.09.19.0001`.
The 15-byte `game.ver` has SHA-256
`3dbeed87ae1f2805bec69d63137e3deca9dd444ad5caa3ea5dedb68b8a50bfef`.

| Input | Encrypted bytes and SHA-256 | Decoded bytes and SHA-256 |
|---|---|---|
| `client/sqwt/widget/PartyParameterWidget.form` | `3586`, `df13dfa51692fdf0c4cf27389d15921d8b8113a0a2f16d0828f0abb44e2acf49` | `3578`, `b88d2ccb850a751aaccf9c2f9247242833b3a27e96e1a359b1cb477c3cf19de1` |
| `client/sqwt/widget/PartyParameterWidget.tpl` | `9369`, `13decf482ae1d3b83fcf98a3ca909402c3d8f4677961b6a237b7e80eed459e08` | `9361`, `cfbdbfd5f592fce7bf16956c91761637acfa1479a229b2b0d3e9db91cd630e34` |
| `client/sqwt/common/default.style` | `353783`, `632b314cb2257fcbee42d4b002989a58dfeb42e64af864c1f5356336dc233dc0` | `353775`, `33bb178c3b172c62eeb65e8bcaf94f701a0ec3921564e27a619516900896d9f6` |
| `client/sqwt/common/WINBasisLight_noGrayout.tpl` | `4303`, `be271e92c52765dd1cfd8c786c0f77144440520b84239f08db5b0c21c97c8949` | `4295`, `47271a67f14e47e0fb458e1a211de9366b17c112576e951668bd519e116ee6d8` |

The SQEX decode used `xivl-tools:tools/blowfish.py`, SHA-256
`7c4208f5d3b7acf20bfb67ddcba706cbb10bf4d1c6c1677b4b11d8685774f90e`.
The producing classifier used Python standard-library ElementTree,
`decimal.Decimal`, `float`, `math.isfinite` and `struct`.

## Carrier observations

Node ordinals are zero-based ElementTree preorder with the document root
included. Category sequences below follow comma-field order only.

| Input | Node ordinals | Component categories |
|---|---|---|
| Party form | `11` | zero, positive, positive, zero |
| Party template | `28`, `29`, `30`, `31` | zero, negative, positive, zero |
| Party template | `32` | positive, zero, zero, zero |

Every listed attribute is unqualified. Each listed element has an XML
namespace and local name `TextBlock`. Every value has exactly three commas
and four nonempty components. All components have integer spelling, with
no decimal point, exponent, unit suffix or percent suffix. The negative
components have a leading minus sign. The other components have no explicit
sign. There is no leading, trailing or interior whitespace in the parsed
values or their components.

Across these carriers the component categories are `4` negative, `7`
positive and `13` zero. No component is a negative-zero literal. Every
component is finite as a Decimal value, a Python binary64 float and after
Python `struct` binary32 conversion.

## Reproduction and limits

Verify the source hashes and SQEX headers. Decode only the identified files
in memory using each exact base filename as the cipher key. Verify decoded
hashes before parsing. Reject DOCTYPE and ENTITY declarations, decode strict
UTF-8-sig and parse with ElementTree.

Enumerate preorder nodes including the root. Select element local name
`TextBlock` and exact attribute key `Margin`, without stripping an attribute
namespace. Split the attribute on commas while retaining empty fields.
Classify the trimmed components by decimal grammar, lexical sign, Decimal
sign/zero and unit suffix. Check finiteness through `math.isfinite(float())`
and little-endian `struct.pack`/`struct.unpack` with format `f`. Keep only
the identities, ordinals and categories above.

XML parsing applies entity decoding and attribute whitespace normalization
before classification. The numeric conversions are instrument checks.
They do not establish a native parser, precision or admission rule.
These observations do not establish property aliasing, owner-name
resolution, inset meaning, layout order, placement, rendering or runtime
behavior.
