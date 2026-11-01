# Sqwt binding converter parameter

`ConverterParameter` supplies a retained string on the observed Sqwt binding
path. The forward conversion branch passes it as the third explicit argument
to an optional converter. Without that converter, the branch copies it into
the working string before reading the source property. A conditional float
property-to-string path uses a nonempty working string as its formatting
argument and supplies the scalar separately as a promoted double.

This establishes a formatting role on that float path. It does not establish
the behavior of every converter, a named converter registry, a parameter
class, a source property slot selected by the parameter, or live HUD output.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | `ghidra` CLI string search, exact xrefs, disassembly and decompilation, plus a read-only PE32 section mapping for the jump table |

All addresses below are VA. Subtract `0x00400000` for RVA. Executable size
and SHA-256 were checked before the observations. Decompiled output supplied
leads. The described loads, stores, arguments and dispatches were checked in
the corresponding instructions. This was static analysis, without client
execution or a fresh analysis-project reproduction. The RangeBase descriptor
indices and special float values use `ghidra-cli 0.2.2` disassembly and
memory reads of the same executable, whose size and SHA-256 were checked
first.

`B` denotes the complete object returned by `FUN_00999c70`. Its constructor
installs table `0x01079CD8` at `B` and table `0x01079C9C` at `B+0x18`,
at `0x00999C78..0x00999C85`. Those receiver positions matter when
interpreting virtual offsets. The [RTTI catalog](../../config/ffxivgame.rtti.json)
identifies both tables as `Sqwt::Data::Binding`.

## Parameter retention and forwarding

`ConverterParameter` occurs at `0x010683B4`. Its exact xref at
`0x009123E6` is in `FUN_00912340`. The parser constructs a key from that
literal, calls `FUN_00913EA0`, and passes the result through the wrapper
helpers `FUN_0099A890` and `FUN_0099A8D0`. The destination receiver at
`0x00912421` is `B+0x68`. The latter helper stores the returned shared
string pointer at its receiver at `0x0099A8DF`. This is string retention,
not an observed converter factory call.

The immediately following `BackConverterParameter` lookup uses the separate
destination `B+0x6C`, at `0x0091248D..0x00912498`.

The forward conversion entry in `FUN_009993C0` requires nonzero `B+0x5C`.
At `0x00999414..0x00999445`, a nonzero converter pointer from `B+0x4C`
selects virtual offset `0x38`. Its four explicit arguments are the dwords
loaded from `B+0x3C` and `B+0x5C`, the string pointer retained through
`B+0x68`, and the working
output string. `FUN_0099A5A0` is the dword dereference used to obtain that
parameter pointer. The source receiver and parameter remain separate inputs.

With a zero converter pointer, `0x00999449..0x00999466` copies the retained
parameter into the working string through `FUN_00447450`, then calls
`FUN_00911020` with the receiver loaded from `B+0x3C`, first argument the
dword loaded from `B+0x5C`, second
argument the working string, and third argument zero. Continuation requires
the returned low byte to equal one at `0x0099946B`.

On its admitted property-info branch, `FUN_00911020` passes the source
receiver, descriptor field `+0x14`, and that same working string to the
selected property-info object's virtual offset `8`, at
`0x0091111F..0x00911130`. Property lookup, any receiver override and
property admission remain conditions. Parameter contents do not choose the
virtual slot in these instructions.

## Conditional float formatting consumer

The [vtable catalog](../../config/ffxivgame.vtable_slots.jsonl) identifies
`Sqwt::DependencyPropertyInfo<Sqwt::Controls::Primitives::RangeBase<float>_>`
table `0x01070304`, slot 2, with adapter `FUN_009A71C0`.
Initializer `0x00F24090..0x00F240E6` selects that table for info object
`0x0135AE40`. It writes callback `0x00966C40` at info offset `0x20` and
zero receiver adjustment at offset `0x24`.

For an info object selected with that adapter and initialization,
`0x009A71C0..0x009A71D8` adds the retained adjustment to the source
receiver and invokes its callback with the descriptor index and working
string. The callback at `0x00966C40` rejects unsigned indices above four
and otherwise dispatches through table `0x00966CD8`. A read-only section
mapping of the identified canonical executable recovered its five dword
targets as `0x00966C54`, `0x00966C6D`, `0x00966C86`, `0x00966C9F` and
`0x00966CB8`, in index order. The 20-byte span maps to file offset
`5663960` in the file-backed `.text` section.

These branches pass an address at receiver offset `0x33C`, `0x344`,
`0x34C`, `0x354` or `0x35C`, respectively, plus the working string, to
`FUN_00988D50`. Out-of-range indices return a false low byte. The
[RangeBase descriptor indices](#rangebase-descriptor-indices) join each
descriptor to its index and receiver offset.

After two scalar-comparison branches, `0x00988DC4..0x00988E0A` tests the
working string's length. A nonempty string supplies its character pointer
as the formatting argument. Empty input selects a separate static literal.
The scalar is loaded as a float and stored as a double on the call stack.
`FUN_0097FD10` receives the local buffer, size `0x100`, the selected format
pointer and the double value. The result is copied back into the working
string by `FUN_004489C0`.

At `0x0097FD10..0x0097FD34`, the wrapper first sets the buffer's last
byte to NUL, then forwards the buffer, size minus one (`0xFF` for this
caller), format pointer and
variadic argument address to `0x009D8CBD`, labeled `__vsnprintf` by Ghidra.
The instructions establish the format and value argument roles. They do not
establish actual output under a particular locale, floating-point environment,
rounding case, malformed format or truncation case. The two earlier scalar
branches are in [Special float values](#special-float-values).

## RangeBase descriptor indices

Five initializers name the `RangeBase<float>` descriptors. Each passes a
name literal and its descriptor to `0x009108E0` and stores a distinct dword
at descriptor `+0x14`. The index store addresses below are that field.

| Index | Name | Name literal | Descriptor | Index store | Initializer | Callback block | Scalar offset |
|---:|---|---|---|---|---|---|---|
| 0 | `LargeChange` | `0x0106FD94` | `0x0135AE1C` | `0x0135AE30` | `0x00F23FA0` | `0x00966C54` | `+0x33C` |
| 1 | `Maximum` | `0x0106FDA0` | `0x0135ADC4` | `0x0135ADD8` | `0x00F23FD0` | `0x00966C6D` | `+0x344` |
| 2 | `Minimum` | `0x0106FDA8` | `0x0135ADA0` | `0x0135ADB4` | `0x00F24000` | `0x00966C86` | `+0x34C` |
| 3 | `SmallChange` | `0x0106FDB0` | `0x0135AD7C` | `0x0135AD90` | `0x00F24030` | `0x00966C9F` | `+0x354` |
| 4 | `Value` | `0x0106FDBC` | `0x0135ADF4` | `0x0135AE08` | `0x00F24060` | `0x00966CB8` | `+0x35C` |

The initializers span `0x00F23FA0..0x00F23FC3`, `0x00F23FD0..0x00F23FF3`,
`0x00F24000..0x00F24023`, `0x00F24030..0x00F24053` and
`0x00F24060..0x00F24083`. The callback blocks are the five targets of the
jump at `0x00966C4D`. Each adds its offset to ECX at `0x00966C59`,
`0x00966C72`, `0x00966C8B`, `0x00966CA4` and `0x00966CBD` and calls
`FUN_00988D50` with that address and the working string. The last column
pairs each block with the offset it adds. STATIC.

The `RangeBase<float>` constructor `0x00967850` calls `0x009C1FD0` once per
descriptor, with the scalar receiver `this+0x33C`, `+0x344`, `+0x34C`,
`+0x354` and `+0x35C` and the descriptors `0x0135AE1C`, `0x0135ADC4`,
`0x0135ADA0`, `0x0135AD7C` and `0x0135ADF4`, at `0x009678C0`, `0x009678E3`,
`0x00967906`, `0x0096792B` and `0x0096794E`. Its pairing agrees with the
table: `Maximum` is the scalar at `+0x344` and `Value` the scalar at
`+0x35C`. The `ProgressBar` default indicator path in
[ProgressBar template style](sqwt-progress-template-style.md) reads the same
three offsets as Value, Minimum and Maximum. STATIC.

The initializer at `0x00F24090` pushes count 5 at `0x00F24099` and calls
`0x0098B250` for info object `0x0135AE40` at `0x00F240AE`. The constructor
passes that info object and the receiver to `0x0098B400` at `0x009679A5`.
That the five descriptors share this one info object and callback is
INFERRED from the count and the single registration. The lookup that maps a binding `Path`
name to a descriptor and then to this info object at run time is not traced
here (UNKNOWN). A `Path=Maximum` binding is therefore the same conditional
path as `Path=Value`, selecting index 1 where `Value` selects index 4. The
source property index is the only difference between the two, and the
`ConverterParameter` handling does not depend on it.

## Special float values

`FUN_00988D50` compares the scalar with two doubles before it reads the
working string. `FLD float [ESI]` at `0x00988D6C` loads the float behind
its first argument, and `FCOMP double [0x00FA2EE0]` at `0x00988D6F` compares
it with `-20000000.0`. The double at `0x00FA2EE0` is the byte run
`00 00 00 00 D0 12 73 C1`. `FNSTSW AX`, `TEST AH,0x44` and `JP` at
`0x00988D7C..0x00988D81` take the jump on every outcome except equality. The
fall-through pushes literal `0x0107729C`, the text `NaN`, at `0x00988D83` and
calls `0x0097FD10` with the buffer, size `0x100` and no value argument.

The second compare at `0x00988D9C..0x00988DA9` uses the double at
`0x0106F978`, bytes `00 00 00 00 D0 12 63 41`, which is `20000000.0`. Its
fall-through pushes literal `0x010772A0`, the text `PositiveInfinity`, at
`0x00988DAB` and calls `0x0097FD10` the same way. Both fall-throughs join
`0x00988E03`, which copies the buffer into the working string.

Any other float continues at `0x00988DC4`. A nonempty working string is the
format and an empty one selects literal `0x010772B4`, the text `%f`, at
`0x00988DEC`. STATIC for the instructions and literals. The equality
reading of the jump is INFERRED from the x87 condition-code bits that the
test selects: the jump is not taken only when C3 is set and C2 is clear.
A float equal to `-20000000.0` or `20000000.0` therefore prints text and
never digits. A NaN compares unordered and an infinity compares unequal, so
both reach the CRT format. Their output is outside this finding.

## Bounded retail HUD carriers

An in-memory SQEX decode and strict UTF-8 XML parse classified a bounded
selection of named forms and their lexical dependency candidates. The decode
used `xivl-tools:tools/blowfish.py`, whose observed SHA-256 was
`7c4208f5d3b7acf20bfb67ddcba706cbb10bf4d1c6c1677b4b11d8685774f90e`.
The parse used Python's standard-library ElementTree. No decoded document,
literal text or Name value is reproduced here.

| Input | Input bytes and SHA-256 | Decoded SHA-256 |
|---|---|---|
| `client/sqwt/widget/PlayerParameterWidget.form` | `5987`, `602cefe3489630240466d9d3dab9484bd59f3dbefaa33a50534f7b65ea5c5b2a` | `88c37e2cc1b7ac5e83f24349dfeeedbdd148a44beaefc41015296ad24a5a8237` |
| `client/sqwt/widget/PartyParameterWidget.tpl` | `9369`, `13decf482ae1d3b83fcf98a3ca909402c3d8f4677961b6a237b7e80eed459e08` | `cfbdbfd5f592fce7bf16956c91761637acfa1479a229b2b0d3e9db91cd630e34` |
| `client/sqwt/widget/ItemHelpWidget.form` | `45183`, `9108081affa967b9946ebc6ac9d90d0944e3193abc73111b5e8310c377b5fb54` | `58f996c347be731f52c10ad728dbd64cb827b327f77bdd2fd94770bdc65b30ef` |

Zero-based preorder XML node ordinals `22`, `28` and `34` in the Player
form and `28` and `30` in the Party template carry inline bindings on
`TextBlock.Text` with the parameter spelling `ConverterParameter=%.f`.
In the XML-parsed attribute values, `%.f` has no single or double quote
delimiter. There is no whitespace around its equals sign or within the
parameter token. The parameter argument follows a space-only separator and
ends immediately before the closing binding brace. These five bindings have
`Path=Value`, `Mode=OneWay` and no explicit `Converter` key.

Each of these five complete `ElementName` values has two colon-separated
segments, both matching `[A-Za-z_][A-Za-z0-9_]*`. The complete value matches
zero same-document Name values. Its first segment matches exactly one
same-document Name value, on a `ProgressBar`. The prefix match does not
establish full-value equality or the native meaning of the colon or second
segment.

ItemHelp ordinals `353`, `368` and `373` carry Binding elements under Label,
with parameters `%.0f`, `%.f` and `%.f`. Each has `Path="Value"`, no explicit
`Converter` key, and an `ElementName` matching exactly one same-document
`ProgressBar` Name value. The inline spelling and colon-segment observations
above are bounded to the identified Player and Party files.

These are XML string-equality and token-syntax observations only. XML parsing
resolves entity references and normalizes attribute whitespace before the
inline syntax classification. The quotation observation concerns delimiters
inside the parsed attribute value, not the XML attribute's outer delimiter.

They do not prove native Name resolution, `ProgressBar` class activation,
selection of the conditional float-info path, the exact `Value` descriptor,
resource loading, event delivery, computed numeric output or live rendering.
They do not establish native quotation handling or acceptance of quoted
parameter alternatives.

## Reproduction and limits

Verify the executable identity and inspect the literal/xref, parser retention,
forward dispatch, property-info adapter, initializer, scalar callback and
format wrapper at the cited addresses. Track actual receivers, every argument
push and callee cleanup. The callback at `0x00966C40` was available as
instructions without a defined Ghidra function. Its disassembly, rather than a
decompiler signature, carries that observation.

For the XML observation, decode only the explicitly identified files with
their exact base names, parse the decoded bytes in memory, enumerate nodes in
preorder and compare complete ElementName strings with Name strings. For the
five inline carriers, identify named key=value boundaries separated by
whitespace or commas outside balanced quotes and nested braces. Retain the
parameter's quote delimiter and whitespace classes. Compare complete
ElementName tokens and their first colon-separated segments separately.
A simple-identifier prefix match cannot establish complete-token equality.
Retain identities, ordinals, tags and syntax classes. Keep client bytes, XML
bodies, executable listings and decompiled bodies outside the public repository.

The surviving gaps are runtime object and table selection, native source
property resolution, the run-time lookup from a `Path` name to its descriptor,
successful downstream property assignment and rendering, all alternate
converter implementations, reverse conversion behavior, the output for NaN and
infinite inputs and exact CRT output rules.
