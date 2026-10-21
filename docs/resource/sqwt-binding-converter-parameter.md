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
execution or a fresh analysis-project reproduction.

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
`FUN_00988D50`. Out-of-range indices return a false low byte. The exact
`Value` descriptor-to-index join is not established here.

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
branches and empty-format literal contents are outside this finding.

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
`TextBlock.Text` with parameter token `%.f`. ItemHelp ordinals `353`,
`368` and `373` carry Binding elements under Label, with parameters
`%.0f`, `%.f` and `%.f`. Each has `Path="Value"`, no explicit `Converter`
key, and an `ElementName` matching exactly one same-document `ProgressBar`
Name value. These are XML equality and token-syntax observations only.

They do not prove native Name resolution, `ProgressBar` class activation,
selection of the conditional float-info path, the exact `Value` descriptor,
resource loading, event delivery, computed numeric output or live rendering.

## Reproduction and limits

Verify the executable identity and inspect the literal/xref, parser retention,
forward dispatch, property-info adapter, initializer, scalar callback and
format wrapper at the cited addresses. Track actual receivers, every argument
push and callee cleanup. The callback at `0x00966C40` was available as
instructions without a defined Ghidra function. Its disassembly, rather than a
decompiler signature, carries that observation.

For the XML observation, decode only the explicitly identified files with
their exact base names, parse the decoded bytes in memory, enumerate nodes in
preorder and compare ElementName strings with Name strings. Retain identities,
ordinals, tags and syntax classes. Keep client bytes, XML bodies, executable
listings and decompiled bodies outside the public repository.

The surviving gaps are runtime object and table selection, native source
property resolution, the exact `Value` index, successful downstream property
assignment and rendering, all alternate converter implementations, reverse
conversion behavior, special scalar behavior and exact CRT output rules.
