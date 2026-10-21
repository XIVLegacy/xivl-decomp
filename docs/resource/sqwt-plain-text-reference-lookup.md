# Sqwt plain text reference lookup

The constructor-selected Sqwt text-reference path can defer a digits-only
`@N` value, look up row `N` in `xtx/_text_ui`, select a field from the
configured language, and supply successful output to the original named
property. TextBlock.Text and inherited Label.Content reach this preprocessing
path through distinct property representations. Admission, loaded schema,
row readiness, selected tables and property-state conditions remain required.

## Input and method

| Item | Identity |
| --- | --- |
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| PE32 identity | Machine `0x014C`, image base VA `0x00400000` |
| Instruments | Ghidra CLI `0.2.2`, Python `3.14.7`, Capstone `5.0.7`, x86 32-bit mode |

The original executable supplied bounded disassembly, decompilation and
file-backed data. Independent decoding checked PE raw-section bounds and
complete instruction-range coverage from instruction starts established by
Ghidra. This is static evidence. It does not establish a live instance,
resource delivery or rendered text.

All addresses are VA. Subtract `0x00400000` for RVA. `I` is the selected
RaptureSqwtInterface receiver, `S=I+0x3C`, and `U` is complete MainModule
plus `0x10`. `C` denotes a complete Control receiver. Placeholder names and
offsets are not original source signatures.

## Conditional reference admission

`xivl-decomp:docs/resource/sqwt-numeric-font-preprocessing.md`, FP-01..03,
owns the common `FUN_00916310` admission conditions. TLS, global-interface
and property-flag branches can retain an input wrapper without reference
lookup. The selected classifier counts encoded `0x02,0x2E` markers. Literal
ASCII `@` followed only by decimal digits therefore selects its `K=0` path.

The selected interface virtual `+0x1C` adjusts `I` to `S` and invokes
`FUN_004CF650`. Its immediate check at `0x004CF7B8..0x004CF7E2`
requires input byte 1 to be ASCII `0` through `9`. It does not validate the
whole remaining string. On admitted normal completion it constructs a
pending record and returns sentinel `0x01356DA8`.

`FUN_004CE9C0` scans the initial segment after `@` to `/` or NUL and
supplies that segment to `FUN_004B5850`. For digits-only input the latter
performs decimal accumulation using 32-bit arithmetic, including leading
zeroes. It does not use the numeric font converter's radix-zero CRT route.
Its backward scan and malformed/overflow cases do not establish a legal
reference domain. This finding restricts its plain form to digits-only `@N`.

Other segment forms, encoded formatting and runtime pickup producers retain
their separate boundaries. In particular,
`xivl-client-structs:docs/desktop-native-call-contract.md`, List text,
records native `@id/i.../s...` serialization. That producer spelling does not
make every arbitrary authored string beginning with `@` a typed pickup.

## Default table and named language fields

`FUN_004CF350` retains its provider at `S+0x268`, constructs the default
wrapper at `S+4` from literal `xtx/_text_ui` at `0x00F8D774`, and calls
its first explicit input's virtual `+0x18`. It dereferences the returned
pointer, indexes dword table `0x00F8D75C`, and stores the selected dword
at `S+0x288`. The exact table entries are `0,1,2,3,4,4`.

Instruction-level argument accounting through `0x0054E190..0x0054E238`
and `0x004DC574..0x004DC5A0` identifies the provider as TextModule at
complete MainModule `+0x564`, or `U+0x554`. The constructor call at
`0x004DC058..0x004DC074` supplies the same ExcelModule input used by the
interface's language getter. TextModule construction installs table
`0x01108F70` and retains that input at TextModule `+0x30`.

ExcelModule table `0x01108C00` has `FUN_00C98150` at virtual `+0x18`.
Its body returns ExcelModule `+0x14`. `FUN_00D34D20` maps the exact
language descriptor strings below to the numeric values in that same field.
The connection is visible at `0x00C98674..0x00C986A7`: a nonempty `lang`
attribute is parsed and compared to ExcelModule `+0x14`.

| Descriptor | Numeric value | Selected reference field |
| --- | --- | --- |
| `ja` | `0` | `0` |
| `en` | `1` | `1` |
| `de` | `2` | `2` |
| `fr` | `3` | `3` |
| `chs` | `4` | `4` |
| `cht` | `5` | `4` |

The descriptor parser returns `0xFFFFFFFF` for an unrecognized spelling.
The six-entry reference table provides no corresponding entry or default.
This mapping does not select a language for an unobserved instance.

On the constructor-selected DOM_Element path, a missing attribute makes
`FUN_0099ADB0` return empty wrapper `0x0137B248` instead of an attribute
value wrapper. The `lang` gate in `FUN_00C98580` compares the returned
wrapper with that empty wrapper through `FUN_00445DB0`. An absent or empty
`lang` therefore takes `0x00C98692 -> 0x00C986A7` without language parsing.
A nonempty value must pass `FUN_00D34D20` and equal ExcelModule `+0x14`.
The separate mode gate at `0x00C9863E..0x00C98674` rejects exact `server`
when module `+0x38` is zero, and exact `client` when it is one. Absent or
empty `mode` passes either gate. These conditions do not require invented
language tags or select a default locale. They also establish no preference
between two admitted definitions. Selected DOM tables, static wrapper
initialization, name lookup, loading and schema conditions remain required.

`xivl-client-data:manifests/sheet_inventory.csv`, `xtx/_text_ui` row,
identifies definition resource `0x0B450082`.
`xivl-client-data:manifests/local_guildleve_names.json`, source
`xtx__text_ui.csv`, pins SHA-256
`23762A422118D99C4144A48B3495C52511B1B1B865E4FC50FDC61A973E3C6ECE`,
`dataRowCount=4355` and `columnCount=5`. These catalog facts do not prove
a selected field's native type or actual resource loading.

## Concrete lookup and loaded-schema condition

`FUN_004CED30` mode zero selects a nonempty pending-record table override
or the default `S+4` wrapper. At `0x004CEE96..0x004CEEC7` it invokes
provider virtual `+0x0C` with the selected table wrapper, row dword,
`S+0x288` field selector, argument vector and output wrapper `S+0x58`.
The selected TextModule target is `FUN_00CA0930`.

That target obtains the table through TextModule `+0x30` virtual `+4`,
then the row through table virtual `+0x18`. Selected ExcelModule target
`FUN_00C981D0` searches its name map and returns a populated ExcelSheet
conditionally. ExcelSheet table `0x0110C6D4` selects `FUN_00CAF760` for
row lookup and `FUN_00CAF330` for field type. The row path delegates to
its backend and requires entry readiness before returning an entry.
The existing population and resource conditions are recorded in
`xivl-decomp:docs/input/player-movement-command-origin.md`, Named source
result and row blocks, and
`xivl-client-structs:docs/autotranslate-selection-contract.md`,
AT-COMP-011..016.

`FUN_00CAF330` loads an index-array pointer from sheet `+0x14` and reads
the mapped byte at that pointer plus the supplied field selector. It loads
a type-array pointer from sheet `+0x10` and returns the byte at that pointer
plus the mapped index. These arrays are reached through stored pointers.
They are not inline bytes at the two sheet member offsets.

The metadata type branch at `0x00CB06FF..0x00CB078C` converts each type
name through `FUN_00CAF390` and stores the result by physical ordinal.
The index branch at `0x00CB078C..0x00CB0824` stores that physical ordinal
`BL` at the parsed external field index in the array pointed to by sheet
`+0x14`. This constructs a logical-field-to-physical-ordinal map. The
source descriptor index sequence is not itself that lookup array.

`FUN_00CAF390` maps schema type-name `str` to native type code `10`.
In `FUN_00CA0930`, type `10` selects `0x00CA09EC`, obtains the row's
mapped field, and invokes TextModule virtual `+8` with its string wrapper,
argument vector and output. The selected continuation is
`FUN_00CA1D60 -> FUN_00CA1D40 -> FUN_00CB2760`.

The code paths alone do not establish a selected backing field. The static
English definition below identifies one schema's map and string type. A
plain-reference consumer must obtain its loaded schema and validate the mapped
field as `str`/type `10` before treating its bytes as the string path. A fixed type
vector, replacement dictionary or default locale is not supplied here.

## Plain and empty output

For NUL-terminated string input without byte `0x02`,
`0x00CB27A4..0x00CB27D5` selects the plain formatter path.
`0x00CB29B7..0x00CB2A12` copies bytes unchanged to a temporary buffer,
including the allocation-growth path, until NUL. The following path appends
NUL and calls `FUN_00447CF0` with the supplied output wrapper. That helper
appends the byte run including its NUL at the output's prior length minus
one. Normal helper and allocation completion remain conditions.

Initially empty input branches directly to the formatter's success exit
without appending. That exit writes `AL=1`. `FUN_00CA1D40` normalizes the
low-byte result into dword Boolean `1`. The formatter alone therefore leaves
an existing output unchanged for empty input. On this pending-reference
path, `0x004CEDDA..0x004CEDE8` has first cleared `S+0x58` through
`FUN_00445530`, so successful empty lookup leaves empty output.

A missing table or row returns zero at the lookup boundary. The pending
consumer advances without erasing or applying that record, preserving its
retry path. A field type outside `1..10` returns `0xFFFFFFFF`, which the
consumer treats as nonzero. Unsupported field type must not be described as
the same failure as a missing row. Encoded byte `0x02` dispatches token
handlers and is outside the plain-copy result.

On nonzero lookup, the consumer assembles any prefix/output/suffix and erases
the pending node. At `0x004CEFBC..0x004CEFD6`, a zero owner copies output
to the saved wrapper from node `+0x138`. Only a nonzero owner reaches generic
property assignment through `FUN_00912850` at
`0x004CEFDB..0x004CEFED`. That property path can conditionally re-enter
preprocessing for returned `@` text. Recursion, cycles and termination are
not established.

## Named text properties

Initializer `0x00F23800..0x00F23824` names Text descriptor `0x0135A300`
and assigns index zero. TextBlock parser `0x009556D0..0x00955764` routes
that index to property `+0x33C` and `FUN_009BA4E0`. Its bit `0x40`
gate controls preprocessing. On a sentinel result it sets bit `0x10` and
returns true in its low byte. Other wrappers reach the conditional string
store through `FUN_00983FE0`.

Initializer `0x00F221E0..0x00F221FA` names Content descriptor `0x01358F94`
and assigns index zero. ContentControl parser `FUN_00936320` routes it to
`FUN_0096CEA0`, which calls the same common preprocessing and marks its
sentinel result. Label constructor `FUN_009742A0` invokes the ContentControl
constructor at `0x009742CA` before installing Label's table. ContentObject
storage remains distinct from TextBlock's string property storage.

Registration precedence, selected runtime objects/tables, allocation/copy
success, property admission, callbacks and rendering remain conditions.
These paths do not prove an external HUD markup occurrence or a particular
binding result.

## Deferred numeric font properties

An admitted deferred FontSize or FontType reference can complete through the
ordinary numeric property path. The pending record preserves the first five
dwords of the original name descriptor, including its qualification and lookup
key. Nonzero-owner completion supplies that saved descriptor and the resolved
wrapper to `FUN_00912850` with mode zero. It re-resolves the registered
property instead of reusing a saved parser index. If the same Control info
object and canonical named descriptor are selected, the returned nonempty
value bypasses the first-byte `{` binding branch, and owner virtual `+0x34`
returns false, FontSize and FontType reach `FUN_0095CB50` at
`C+0x2E0` and `C+0x2D8`. For a returned plain non-`@` wrapper classified
as zero, the constructor-selected preprocessing path can return that wrapper
even while property bit `0x10` is set, provided its post-helper tests still
see the plain input. It then reaches the existing signed converter.
`FUN_00926E80` performs deferred cleanup and clears bit `0x10` before its
state gates. With mode zero, a clear post-cleanup bit `0x02` admits the
ordinary value comparison. An admitted changed value stores the complete
converter dword at `0x00926EE4`. Equal values skip the store. Lookup success
therefore does not guarantee a changed property. Loaded source, name lookup,
helper completion, selected tables, property admission and later callback/font
resource conditions remain required.

The converter remains described by
[signed numeric conversion](sqwt-markup-font-numeric-properties.md#fm-03-preprocessing-and-signed-numeric-conversion).
The later [FontSize bridge](sqwt-font-size-property-bridge.md) retains its
callback, copy, cache, allocation and bitmap-branch conditions.

## Static English definition

For the identified retail build, master resource `0x01030000` names
`xtx/_text_ui` through definition resource `0x0B450082`. The latter has one
explicit `lang="en"`, `mode="client"` definition, at zero-based sheet ordinal
`1`. Its sole physical column is `str`, and its source index sequence is
`[1]`. The native index construction therefore maps logical field `1` to
physical ordinal `0`. Together with the named-language mapping and `str`
conversion above, the explicitly selected English field reaches native string
type `10`. This establishes a concrete static schema input. It does not select a
runtime locale or mode, prove that a running instance loaded these resources,
or establish the contents or rendering of a selected row.

The original master is 7044 bytes, SHA-256
`860023878f9ad1e98fd1baa3e3380f971f86bf1f7747043cf0c3c7459cee2eb4`.
The definition is 3099 bytes, SHA-256
`27e50caf3d258e8eed2d9e70bc8355e2ab67f54f2dc8bf54e6cfcdf604e5e492`.
Decoded master span `[1706,1755)` selects the reference at ordinal 32.
Decoded definition span `[670,1272)` selects the English sheet at ordinal 1.

Python 3.14.7 and `xivl-tools:tools/research/census_sheet_stack.py`,
`decode_document`, recovered the two XML documents in memory. Independent
byte-position decoding, exact re-encoding and Expat span checks agreed.
The decoded master SHA-256 is
`508ae64ce9d60c18241dba3200ba317157f086e201fe0b0fce3aeb807c9ec113`.
The decoded definition SHA-256 is
`2763f4ae79453b368b74f844f9a53647c6788e1c37a327258fd297327f799bac`.
No string row content was decoded for this schema observation.

## Reproduction bounds

Verify the executable identity, PE32 base and section raw-file bounds.
Read selected table slots as little-endian dwords. Track each push, receiver
adjustment and callee cleanup. Inferred constructor parameter lists alone
misidentify the provider. Keep executable bytes, generated listings and
function bodies outside published artifacts.

| Purpose | Instruction ranges or exact data reads |
| --- | --- |
| Admission and initial decimal parse | `0x004CF7B8..0x004CF7E2`, `0x004CEA5E..0x004CEB58`, `0x004CE582..0x004CE5B6`, `0x004B5850..0x004B58AF` |
| Table/provider construction | `0x004CF350..0x004CF44C`, `0x0054E190..0x0054E238`, `0x004DC574..0x004DC5A0`, `0x004DC058..0x004DC074`, `0x00CA03DF..0x00CA03EE` |
| ExcelModule and language | `0x004B33E6..0x004B33FF`, `0x004B367E..0x004B369C`, `0x004DC3A0..0x004DC419`, `0x00C98150..0x00C98154`, `0x00D34D20..0x00D34DB9`, `0x00C98674..0x00C986A7` |
| Language initializers | `0x00F2D560..0x00F2D581`, `0x00F2D590..0x00F2D5B1`, `0x00F2D5C0..0x00F2D5E1`, `0x00F2D5F0..0x00F2D611`, `0x00F2D620..0x00F2D641`, `0x00F2D650..0x00F2D671`, `0x00F2C6D0..0x00F2C6E5` |
| Lookup and completion | `0x004CEE4C..0x004CEEC7`, `0x004CEF48..0x004CF006`, `0x00C981D0`, `0x00CAF330..0x00CAF345`, `0x00CAF760..0x00CAF7AE`, `0x00CA0930..0x00CA0A25`, `0x00CA0D79..0x00CA0DF6`, `0x00CA1D40..0x00CA1D86` |
| Schema type mapping and field map | `0x00CB06FF..0x00CB078C`, `0x00CB078C..0x00CB0824`, `0x00CAF390`, especially `0x00CAF4D2..0x00CAF540`. Type-name base `0x01376E98`, stride `0x54`, `str` wrapper `0x013771E0`, literal `0x0110C56C` plus 4 bytes |
| Plain/empty formatting | `0x00CB2760..0x00CB27B5`, `0x00CB27CC..0x00CB27D5`, `0x00CB2951..0x00CB2AB4`, `0x00447CF0..0x00447D38`, `0x00445530..0x00445548`, `0x004CEDDA..0x004CEDE8` |
| Saved name and numeric re-selection | `0x00910750..0x00910775`, `0x004CE9EB..0x004CE9F8`, `0x004CECA7..0x004CECCC`, `0x004CEF5D..0x004CEF7F`, `0x0091293E..0x0091297D`, `0x00912A2B..0x00912A57`, `0x0091B440..0x0091B460`, `0x009382F8..0x0093836E` |
| Deferred numeric preprocessing and store | `0x0095CB50..0x0095CBC7`, `0x00916374..0x0091640C`, `0x00916420..0x00916461`, `0x004CF6C7..0x004CF6ED`, `0x00988A30..0x00988A8A`, `0x00926E80..0x00926F06` |
| Byte-buffer growth | `0x008CFB00..0x008CFB82`, `0x00444E80..0x00444F9C`, `0x006D1920..0x006D194E` |
| Text/Content consumers | `0x00F23800..0x00F23824`, `0x009556D0..0x00955764`, `0x009BA4E0..0x009BA531`, `0x00F221E0..0x00F221FA`, `0x00936320..0x00936352`, `0x0093648C..0x009364B2`, `0x0096CEC6..0x0096CF06`, `0x009742C4..0x009742F8` |
| Exact tables and literals | `0x00F8D75C` plus 24 bytes, `0x00F8D774` plus 13 bytes, `0x01108C00` plus 28 bytes, `0x0110C6D4` plus 28 bytes, `0x01108F70` plus 40 bytes, `0x01110654` plus 24 bytes, `0x00CA0DF8` plus 40 bytes |

The surviving gaps are actual runtime-selected schema and row content,
resource delivery, encoded formatting and full argument substitution,
malformed-reference behavior, returned-reference recursion, native pickup
route equivalence, callback activation and final rendered output. Passing
static checks does not settle those separate conditions.
