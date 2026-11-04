# Excel row cells and native f16 conversion

Source: retail `ffxivgame.exe` 1.23b, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 x86, image base `0x00400000`. Addresses are VA. Capstone 5.0.7
x86-32 and pefile 2024.8.26 qualified original bytes, dispatch metadata and
entry-rooted syntactic normal control flow. This is static evidence.

## Row construction and cell storage

Constructor `0x00C9A810` receives ECX=row and stack arguments
`(inputPointer,parentSheet)`. It installs table `0x01108D24`, clears row
readiness byte +4 and cell pointer +8, and stores parentSheet at row+0x0C
at `0x00C9A82C`. Call `0x00C9A82F` invokes decoder `0x00C9A6A0` with
the same row receiver and inputPointer. The constructor returns its held
row with RET 8 at `0x00C9A837`, under call/register preservation conditions.

The decoder first calls opaque `0x00C9A600(row)` at `0x00C9A6A4`.
The following field observations apply to the row/parent storage present
after that call's ordinary return. They do not independently prove the
helper's preservation behavior.

The decoder reads unsigned byte count n from parent+0x0A, requests `4*n`
bytes through `0x009D04AC` at `0x00C9A6C1`, and stores the returned cell
pointer at row+8. For internal slot i it reloads the parent from row+0x0C,
reads `parent.typeBytes[i]` through parent+0x10, and dispatches `type-1`
through the eleven-dword table at `0x00C9A7E4`. Types outside 1..11 skip
field consumption and continue the slot loop. Ordinary completion writes
readiness byte row+4=1 at `0x00C9A7D8`; the decoder returns with RET 4.

| Type | Branch VA | Input bytes consumed | Store at `cells+4*i` |
| --- | --- | ---: | --- |
| 1, bool | `0x00C9A6FD` | 1 | Low byte copied without Boolean normalization |
| 2/3, s8/u8 | `0x00C9A70D` / `0x00C9A71D` | 1 | Low byte copied |
| 4/5, s16/u16 | `0x00C9A72D` / `0x00C9A73F` | 2 | Low word copied |
| 6, s32 | `0x00C9A74E` | 4 | Full dword copied |
| 7/9, u32/float | `0x00C9A75B` | 4 | Full dword copied |
| 8, f16 | `0x00C9A768` | 2 | Converted four-byte floating-point value |

The shorter stores do not establish the upper bytes of a cell. Source scalar
loads use x86 little-endian memory access. This is a four-byte *cell stride*,
independent of source width and each branch's store width.

The type numbers/names and metadata construction reuse
`xivl-client-structs:docs/spreadsheet-native-contract.md`, the schema-name
lookup `0x00CAF390` and sheet constructor `0x00CB04B0`. That source records
type bytes at sheet+0x10, column mapping at +0x14 and stored width 2 for
type 8. The Lua SpreadSheet getter's type-8 Nil result is a separate output
representation, without a bearing on this native row decoder's float cell.

## Type-8 conversion

At `0x00C9A768..0x00C9A76C` the branch zero-extends the source word and
passes it as one stack argument to `0x00D41110`. Incoming ECX still holds
the parent sheet, but the helper does not use ECX as a receiver. It consumes
the low word of stack argument 1 and returns through x87 ST0. The caller
cleans four argument bytes, stores ST0 with FSTP dword at `0x00C9A777`,
and advances its source pointer by two at `0x00C9A77A`.

For source word h, the helper constructs these binary32 bits:

```text
if h == 0: bits = 0
otherwise:
  bits = ((h & 0x8000) << 16)
       | (((h >> 10 & 0x1F) + 0x70) << 23)
       | ((h & 0x03FF) << 13)
```

The entire-word zero test at `0x00D41115` selects FLDZ. Every nonzero
word instead receives exponent rebias +0x70 at
`0x00D41122..0x00D4114C`, followed by FLD dword at `0x00D41152`.
There is no separate exponent-zero or exponent-31 classification. All
resulting values are finite binary32 numbers, exactly representable through
the selected x87 load/store path.

The conversion agrees with normalized IEEE binary16 values and positive
zero. It does not implement IEEE binary16 subnormal, infinity, NaN or
negative-zero handling. These are synthetic input-word examples, rather
than observed retail row values:

| Source word | Stored binary32 bits | Result |
| --- | --- | --- |
| `0x0000` | `0x00000000` | Positive zero |
| `0x0001` | `0x38002000` | `2^-15 * (1 + 2^-10)` |
| `0x3C00` | `0x3F800000` | 1 |
| `0x8000` | `0xB8000000` | `-2^-15` |
| `0x7C00` | `0x47800000` | 65536 |
| `0x7FFF` | `0x47FFE000` | 131008 |

## Applicability and evidence bounds

The [row accessor](../actor/appearance-path-decoder.md#request-and-delivery)
returns `[row+8]+4*map[column]`, with map at `[[row+0x0C]+0x14]`.
Its argument is a logical sheet column; the decoder loop's i is an internal
slot. Their association requires the actual parent's column map.
[WeaponVisual's selected columns](../actor/weapon-visual-row-attributes.md)
apply this contract conditionally to a delivered `var_wep` row. The selected
definition's identity index list does not prove that any current or historical
request used those exact metadata/source bytes.

The decoder receives no source-span argument. The selected CFG establishes
no source-end or metadata-extent check, allocation-failure guard or validation
condition before the readiness store. Valid storage, successful allocation,
ordinary returns, correct metadata, preservation and lifetime remain premises.
Readiness alone does not establish successful validation.

| Complete VA span, exclusive end | Bytes | SHA-256 |
| --- | ---: | --- |
| `0x00C9A6A0..0x00C9A7E1` | 321 | `cb68125528e6ea56a6c8f2c627c3a9f7ca36a7bd92ab3593149e139454a0f595` |
| `0x00C9A810..0x00C9A83A` | 42 | `fffbb741de3d15c4230109b491094843e2ade64d4b585daa8ae611549cc4a2c7` |
| `0x00D41110..0x00D41157` | 71 | `b94b61e2d222fa206cbf58003a144448e2df20dec48ccea3005b0ac618c8b2ce` |

Bounded decoding and dispatch-table traversal cover all 111 decoder
instructions and all constructor/conversion-helper instructions. A static
interpreter of the helper's admitted integer/FLD subset agrees with the
formula for all 65536 input words. This checks the recovered conversion;
it is not execution of the client or evidence that those words occurred in
retail data. The helper makes no calls. No original body listing, executable,
row bytes or runtime capture accompanies this finding.
