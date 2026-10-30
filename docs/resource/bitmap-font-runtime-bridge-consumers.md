# Bitmap font runtime bridge consumers

The observed bitmap construction path retains a caller-supplied raw scalar.
The resource path preserves payload insertion order on its available-capacity
append branch and consumes TEXC as a second-resource index offset. The
embedded text processor joins the composite scalar getter to literal newline
stepping. Configured font size, half-size resource grouping, baseline
placement, and a universal line-height contract remain unresolved.

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
| Producer | ghidra-cli `0.2.2`, Ghidra `12.1.4`, JDK `26.0.2.1` |
| Independent file decoder | Python PE32 section mapping and Capstone `5.0.7` |

File size, SHA-256, PE32 magic, and image base matched this identity. A new
read-only Ghidra script checked the stored program name, executable SHA-256,
image base, and language. Each bounded instruction and data span used below
had identical bytes in the mapped file and stored program. Capstone decoded
the file-backed instructions. Ghidra supplied bounded stored-memory reads,
listing data, and xrefs. The existing analyzed program was reused. This was
not a fresh import, retail-input CI run, or live rendering test.

The [Ghidra Program API](https://ghidra.re/ghidra_docs/api/ghidra/program/model/listing/Program.html)
and [Capstone Python interface](https://www.capstone-engine.org/lang_python.html)
define the instruments. The executable supplies the retail evidence.

All addresses are VA. Subtract `0x00400000` for RVA. Observations have
confirmed confidence only within the cited bounds. Placeholder function names
and notation do not imply recovered source-level member names or signatures.

The existing glyph and metric findings own these bases: complete FontData
`F`, callback `C=F+8`, complete BitmapFont `B`, draw interface `B+4`, raw
dword `R` at `B+0x220`, and FontData pointer at `B+0x228`.

## BR-01: Caller scalar and source selector

Observation: the bitmap branch of `FUN_00C97150` tests byte `M+4` for zero
at `0x00C97179` and allocates `0x22C` bytes at `0x00C97258`. Its first
explicit argument is retained through the resolver call at `0x00C97283`
and passed as the second explicit argument to `FUN_00CAD030` at
`0x00C9728B`. The resolver result becomes the constructor's first argument.
The constructor stores those arguments at `B+0x220` and `B+0x228` at
`0x00CAD048` and `0x00CAD071`. The first module argument reaches `R`
without scaling, numeric conversion, clamping, or default substitution in
this observed handoff. `M` denotes the module receiver.

Both cache-miss paths `FUN_00C97390` and `FUN_00C973C0` preserve their two
explicit argument patterns at calls `0x00C973AC` and `0x00C973DC`.
The latter returns zero on construction failure or adds 4 to expose the draw
interface at `0x00C973E7`. Wrapper `FUN_004B2CB0` maps only its second
argument from exactly 3 to zero at `0x004B2CB4` through `0x004B2CBB`,
then transfers to `FUN_00C973C0`. Its first argument is unchanged.
A bounded xref query found only vtable data `0x00F8CBD8` for this wrapper.
Its virtual caller and the configured source of `R` remain unresolved.

`FUN_00C96D90` indexes the dword array at `M+0x14` at `0x00C96DB6`
through `0x00C96DBD`. For a positive signed index, it searches earlier
entries and reuses the first matching earlier index at `0x00C96DC2`
through `0x00C96DD8`. The resulting index selects the pointer table at
`M+0x1C` at `0x00C96DDA` through `0x00C96DE4`. The indexed source
load precedes that loop and has no bounds test in this window.

On a missing pointer, the selected source-table dword is passed either to
slot 1 of the nonzero global object at `0x0137603C`, at `0x00C96E17`
through `0x00C96E20`, or to the FontData constructor `FUN_00CA6290` at
`0x00C96E4C`. The latter also receives zero-extended byte `M+0x10`.
This establishes the source-data selection and reuse mechanism. It does not
name the table dwords as sizes, assets, or numeric families.

Interpretation: `R` is a caller-supplied raw scalar, distinct from the
argument that selects FontData. Its user-facing unit, origin, legal range,
fallback, and default are not established. A separate table-choice lead at
`0x004B34FA` and `FUN_004B2B80` does not close those gaps.

## BR-02: Payload order and second-resource offset

Observation: the `FUN_00CA65C0` DDSF loop precedes its GTEX loop. Tag
operands are at `0x0110B37C` and `0x0110B384`, with comparisons at
`0x00CA66CF` and `0x00CA679F`. Each matching path passes current chunk+8
to `FUN_00813050` using `C+0x38`, complete `F+0x40`, at
`0x00CA6706` through `0x00CA6719` and `0x00CA67D6` through
`0x00CA67E9`. The helper's available-capacity branch stores the supplied
dword at the end and advances the end by four at `0x00813079` through
`0x00813087`. On that branch, matching payloads retain their visited order.
The helper's growth path is outside this observation.

The creation loop at `0x00CA63D0` through `0x00CA644A` starts with
index zero, increments by one, and continues with unsigned less-than.
It passes each selected payload to resource construction and appends the
returned pointer through `FUN_0096C140` using the vector at `F+0x44`.
The latter's available-capacity branch stores a dword and advances by four
at `0x0096C169` through `0x0096C177`. This joins the payload list to
resource begin/end fields `F+0x48` and `F+0x4C`. No group split or
half-size selection occurs in this bounded creation loop.

Unsigned `F+0x18` at least 4 admits the TEXC path at `0x00CA683B`
through `0x00CA683F`. The tag operand is `0x0110B38C`. The load at
`0x00CA68A6` and store at `0x00CA68AD` preserve chunk+8's raw dword
in `C+0x14`, complete `F+0x1C`, without numeric conversion or byte swap.
Construction clears `F+0x1C` at `0x00CA6302`. The lower-version branch
skips the store and does not itself reset retained state. A tag mismatch
reports before joining the store path if the reporting call returns.

On the reached plain draw path, normalized resource byte 0 is sign-extended
at `0x00CAC564`. The call to `FUN_00CAD120` at `0x00CAC5BE` uses
that index plus `F+0x1C`, retaining the low 32 bits. The second call at
`0x00CAC5D0` uses a freshly sign-extended byte 0 without the offset.
There is no intervening flag-`0x02` branch between these two accessor
calls. The unoffset result becomes original dispatch argument 6, and the
offset result becomes argument 7 at the call `0x00CAC5F8`.

The accessor computes its bound with a 32-bit end-minus-begin subtraction
followed by an arithmetic right shift by two at `0x00CAD12B` through
`0x00CAD134`. Its unsigned comparison rejects an index at least that bound
at `0x00CAD137` through `0x00CAD139`. A null begin or rejected index
returns zero. A passing index selects a four-byte entry at `0x00CAD150`
through `0x00CAD153`. Negative sign-extended glyph indices therefore
retain their unsigned bounds-check consequences, subject to the preceding
32-bit offset addition.

On the reached dispatcher branch through `FUN_00CA55A0`, caller flag
low-byte bit `0x02` is tested at `0x00CA55B9`. The clear branch replaces
the offset-resource argument with zero at `0x00CA55F7` before forwarding
to `FUN_00CB9280` at `0x00CA5659`. The set branch skips that replacement.
Including the dispatcher's additional first argument, the overwritten entry
is its eighth explicit argument at entry-relative `0x20`, original argument
7. Other dispatcher and backend branches remain outside this observation.

Interpretation: TEXC supplies an index offset on this path. These native
relations alone do not identify a font member. Runtime asset identity, creation
or binding success, half-size atlas grouping, dimension-halving selection,
and a relation between TEXC and half the list length remain unresolved.

### Selected TEXC dimension pair

The [retail resource census](bitmap-font-container.md#retail-resources)
pins `data/27/95/00/02.DAT`, size 823938, SHA-256
`aa022ebd8610ae8ba0dfa6b4c356b58c32c09945d8f4452d73157debcc26b804`.
A bounded chunk walk of that original file reads TEXC as 1 and finds these
two GTEX payloads, in file order:

| Ordinal | File payload offset | Bytes | Requested width/height | Payload SHA-256 |
|---:|---:|---:|---|---|
| 0 | `20` | `655408` | `2048 / 128` | `ebdca9899e6e35326564f7292ca8eef0072981eafe32638dbcc013e38dcbf101` |
| 1 | `655436` | `163888` | `1024 / 64` | `7963a2df347aead9cddc5b9a295cf1b65049c817fdd21c60356a55ba91733889` |

Both descriptors use format index 22. The second descriptor stores half
the first descriptor's width and height. This is an authored data relation,
not an automatic halving rule established by TEXC. With index zero and the
resource order above retained, the unoffset and TEXC-offset requests select
ordinals zero and one respectively.

The [texture consumer](bitmap-font-texture-consumers.md#resource-identity-and-requested-dimensions)
normalizes coordinates by the first wrapper's requested dimensions at
`0x00CB9551..0x00CB957F`. The
[second-resource shader path](bitmap-font-edge-shader-consumers.md#bs-03-complete-pixel-interpretation)
uses the same normalized coordinates for both samplers, without
renormalizing against the second wrapper's dimensions. Together these
facts explain how unequal stored dimensions can participate in that
conditional path; they do not prove which font instance selected it.

Python 3.12.0 standard-library reads checked the original file hash, each
little-endian chunk length and whole-file consumption, then decoded the
GTEX big-endian width/height words in place. Original-PE Capstone 5.0.7
normal-CFG checks covered `0x00CAD120`, `0x00CA55A0` and `0x00CB9280`
without unresolved local branches. Runtime source identity, retained vector
order, flag `0x02`, wrapper creation, stage binding and rendered output
remain unobserved. The next static join is from the exact FontData source
to this member and its retained resource vector. A live discriminator would
record both selected wrappers and extents at `0x00CAC5BE/0x00CAC5D0`,
the flag at `0x00CA55B9` and UVs at the `0x00CB962E` batch submission.
Correlate that submission with its eventual flush through `0x00CB90D0`,
recording the stage-0/1 resource arguments at
`0x00CB917C/0x00CB918A`; actual binding success is a further observation.

## BR-03: Composite getter and literal newline increment

Observation: the base construction called at `0x00CAD033` passes complete
`B` to `FUN_00CC14B0` with receiver `D=B+0x10` at `0x00CC05D4`
through `0x00CC05DF`. That constructor retains `B` at `D+0x54` at
`0x00CC150B`. BitmapFont installs vtable `0x0110C0BC` at `B+0x10`
at `0x00CAD062`. Slot 4 at `0x0110C0CC` contains `FUN_00CC0C40`.

Primary BitmapFont slots 11 and 12, table entries `0x0110C1C0` and
`0x0110C1C4`, select wrappers `FUN_00CBFE20` and `FUN_00CBFE30`.
Each adds `0x10` to its receiver before transferring to `FUN_00CC1FE0`
or `FUN_00CC2190`. Each setup calls complete-font virtual offset 8 through
`D+0x54` at `0x00CC1FFD` or `0x00CC21B5`. With the observed primary
vtable, this selects slot 2 at `0x0110C19C`, `FUN_00CABF90`, the
published composite scalar getter.

The returned integer is stored at `D+0x6C` at `0x00CC2003` or
`0x00CC21BB`. Each setup adds raw parameter dword `P+0x2C` and stores
the low 32 result bits at `D+0x68`, at `0x00CC2006` through
`0x00CC200D` or `0x00CC21BE` through `0x00CC21C3`. `P` denotes the
caller-provided parameter pointer. Both setups clear `D+0x70` and
`D+0x74` before calling text processor `FUN_00CC1C50` at
`0x00CC20F0` or `0x00CC2276`.

The text processor reads the current byte at `0x00CC1D50`. Its CRLF test
at `0x00CC1DFF` through `0x00CC1E07`, and isolated LF or CR tests at
`0x00CC1E36` through `0x00CC1E3C`, dispatch through `D`'s virtual
offset `0x10` at `0x00CC1E1E` or `0x00CC1EB3`. The installed table
joins these calls to `FUN_00CC0C40`. Its dword addition at `0x00CC0C66`
through `0x00CC0C69` adds `D+0x68` to `D+0x74`, retaining the low
32 bits. It also increments `D+0xB4` and clears `D+0x70` at
`0x00CC0C72` and `0x00CC0C7E`. No additional float scaling or
truncation occurs in that increment.

Interpretation: the composite getter result plus `P+0x2C` supplies the
literal newline increment on this construction and setup path. This does not
establish the parameter's semantic name or default, geometric sign convention,
a universal line height, every text encoding or control opcode, or every
rendering caller. No baseline-placement join from the BSLN-derived slot-3
getter was established in this bounded inspection.

## Reproduction bounds

Keep raw observations local. Use the identified executable and stored program,
check their identities, compare file-backed and stored bytes, and account for
stack changes and subobject adjustments.

| Purpose | Instruction or data bounds |
|---|---|
| Constructor argument handoff | `0x00C97150` plus `0x40`, `0x00C97258` plus `0x50`, `0x00CAD030` plus `0x50` |
| Cache and wrapper chain | `0x00C97390` plus `0x30`, `0x00C973C0` plus `0x40`, `0x004B2CB0` plus `0x18`, xrefs to `0x00CAD030` and `0x004B2CB0` |
| Source selector and table-choice lead | `0x00C96D90` plus `0x110`, `0x004B34F1` plus `0x22`, `0x004B2B80` plus `0xA0` |
| FontData bases and payload loops | `0x00CA6290` plus `0x97`, `0x00CA65C0` plus `0x90`, `0x00CA66A0` and `0x00CA6770` each plus `0xD0`, `0x00813050` plus `0x50` |
| Resource loop and TEXC | `0x00CA63D0` plus `0x80`, `0x0096C140` plus `0x40`, `0x00CA683B` plus `0x80`, data `0x0110B37C` plus 20 |
| Draw resource indices and dispatcher | `0x00CAC330` plus `0x60`, `0x00CAC564` plus `0x99`, `0x00CAD120` plus `0x40`, `0x00CA55A0` plus `0xC0`, `0x00CA59E0` plus `0x60` |
| Embedded text object and vtables | `0x00CC0580` plus `0x80`, `0x00CC14B0` plus `0x70`, `0x00CBFE20` and `0x00CBFE30` each plus 8, data `0x0110C0BC` plus 24 and `0x0110C194` plus 76 |
| Setup and composite getter | `0x00CC1FE0` plus `0x130`, `0x00CC2190` plus `0x110`, `0x00CABF90` plus `0x81` |
| Literal newline | `0x00CC1C50` plus `0x70`, `0x00CC1D50` plus `0xB0`, `0x00CC1DFF` plus `0x55`, `0x00CC1E9E` plus `0x32`, `0x00CC0C40` plus `0x80` |

Decompiler requests for `0x00CC14A0` and `0x00CC1400` returned
`No function at address`. The actual `0x00CC1400` callback pointer is
visible at `0x00CC20E2` and its bounded instructions are readable.
The newline finding uses instruction and data evidence. These lookup failures
are not successful decompiler reproductions.

Full selection policy, legal argument and flag domains, caller defaults, all
styles, original atlas resources, device precision, floating-point control
state, malformed input, complete layout, runtime state, and raster output
remain unresolved.
