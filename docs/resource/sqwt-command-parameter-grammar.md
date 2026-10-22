# Sqwt CommandParameter text grammar

`FUN_0099DDF0` turns the text of a Sqwt `CommandParameter` into a fixed
`0x78`-byte argument record. It accepts three families of text: an empty
string, a brace form that starts with the bytes `{Co`, and a plain form that
is either an integer or an arbitrary string. The parser never fails. A text it
does not understand leaves the record unchanged or reduces it to a string. The
record is then carried unchanged to the routed command dispatcher.

This page records the accepted grammar, the record layout, the callers that
feed the parser, and the path that carries the record to command execution.
It does not establish what any individual command does with its arguments.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | `ghidra` CLI decompilation, exact xrefs and string search as leads. Capstone `5.0.7` x86 32-bit disassembly over a read-only PE32 section mapping of the identified executable as the confirming read. |

All addresses are VA. Subtract `0x00400000` for RVA. Executable size and
SHA-256 were checked before the observations. Every instruction-level claim
below was read from the Capstone disassembly, not from decompiled output.
This was static analysis. It did not execute the client.

Evidence labels used below:

- STATIC: read directly from instructions or data at the cited address.
- INFERRED: a reading that joins static facts with a convention, such as a
  standard C runtime routine or a caller's role. The join is stated.
- UNKNOWN: not established.

## Parser entry and record layout

`FUN_0099DDF0` spans `0x0099DDF0..0x0099E0EE` and returns with `ret 4`. The
receiver in ECX is the record `R`. The one stack argument is the text `T`, a
Sqwt string object. The string class is 0x54 bytes with the data pointer at
`+0x00`, an inline capacity dword of `0x40` at `+0x04`, length plus one at
`+0x08`, and a UTF-8 aware character counter behind positions in its find
helpers (`0x00445E50`, `0x00446B10`). Accessors `0x00445210` (data pointer)
and `0x004451F0` (byte length, `+0x08` minus one) are one-instruction
readers. STATIC.

| Record offset | Width | Use |
|---|---|---|
| `+0x00` | dword | Kind `K`. `0` none, `1..` count of integers, `8` string. |
| `+0x04` | string object, `0x54` bytes | Text of a string argument. |
| `+0x58..+0x74` | eight dwords | Integer arguments. |

STATIC for the layout and the size. Constructor `0x0052D330` clears `+0x00`,
builds the string at `+0x04` and clears the eight dwords at `+0x58..+0x74`
(`0x0052D359..0x0052D37E`). The allocation that precedes it pushes `0x78` at
`0x00953EDC`. Callers that build a record on the stack zero eight dwords
(`0x00954330`, `0x00954500`). The binding markup object stores its template at
markup offset `+0xCC` and the next field, the command target, at `+0x144`, a
`0x78`-byte gap.

The kind field is shared between a count and a tag. `K == 8` means a string,
so an integer list of exactly eight tokens is indistinguishable from a string
record by kind alone. INFERRED from the two writes of `K`.

## Accepted text

The parser reads the first byte of `T` and chooses a path
(`0x0099DE3D..0x0099DE46`, `0x0099E042..0x0099E04C`).

### Empty text

If the first byte is NUL, `K` becomes `0` and nothing else is written
(`0x0099E046`). STATIC.

### Brace text

A text that starts with `{` takes the brace path. The parser compares the first
three bytes of `T` with the first three bytes of the literal `{CommandParameter`
at `0x0107AC74` through `_strncmp` `0x009D5475` with count 3
(`0x0099DE4C..0x0099DE66`). A nonzero result jumps to the common exit at
`0x0099E0B4` and the record is untouched. So `{Co` followed by anything passes
the gate. Text such as `{Binding` does not pass and is ignored by this
function. STATIC.

The brace path then has two sub-forms, tried in order.

String form (`0x0099DE6C..0x0099DF24`). `strchr` (`0x009D8510`) finds the first
`,` in `T`. If one exists, the bytes before it are copied into a temporary and
searched with `strstr` (`0x009D6090`, haystack first) for the case-sensitive
needle `String` at `0x0107AC88`. If found:

- `K` becomes `8`.
- The string at `R+0x04` is assigned the bytes from one past the comma up to
  but excluding the last byte of `T`. The length passed to the string
  constructor `0x00447260` is `end - (comma + 1) - 1`.

The last byte is dropped without testing that it is `}`. Leading whitespace
after the comma is kept. Later commas are kept because only the first comma is
located. A comma that is the final byte gives length `-1`, which `0x00447260`
treats as `strlen` of an empty remainder. STATIC for the arithmetic and the
`-1` handling at `0x00447260`.

Token form (`0x0099DF40..0x0099E02C`). This runs when no comma exists or the
text before the first comma does not contain `String`. The parser builds the
delimiter string at `0x0107AC90`, which is the three bytes `}`, space and
`,`. It calls tokenizer `0x0044F770`, which loops
`find_first_not_of` (`0x00446FB0`) then `find_first_of` (`0x00446F90`), takes
the span as a token (`0x00447A80`) and appends it (`0x0044E950`). Runs of
delimiters collapse and no empty token is produced. Each token is one `0x54`
byte string, so element `i` of the vector sits at `i * 0x54`
(`0x00444CB0`). STATIC.

With `n` tokens:

- `n < 2` leaves the record untouched (`0x0099DFBB..0x0099DFBE`).
- Token 1 is the type. `__stricmp` (`0x009D244E`) against `Int` at
  `0x0107AC94` decides. Case is ignored.
- If the type is `Int`: `K = n - 2`, and for `i` in `0..n-3` the dword at
  `R + 0x58 + 4*i` receives the result of calling `FUN_00988A30` with the
  arguments `(&R[0x58 + 4*i], token[i + 2])` (`0x0099DFE6..0x0099E01A`, call at
  `0x0099E006`). The converter ignores the destination pointer. Token 0 is
  never read after the three byte gate.
- For any other type, the parser calls `__stricmp` against `None` at
  `0x0107AC98`, ignores the result and sets `K = 0` (`0x0099E01C..0x0099E02A`).
  `None`, an unknown word and a misspelled `Int` are indistinguishable. The
  string at `+0x04` and the integers are not cleared.

Because comma and space are both delimiters, `Int` lists accept either
separator in any mix. The string form is the only place where a comma has
special meaning. STATIC.

### Plain text

Any other first byte takes a three state scan over `T`
(`0x0099E04E..0x0099E0A0`):

| State | Byte | Action |
|---|---|---|
| 0, leading | space (`0x20`) | advance, stay in state 0 |
| 0 | any other | go to state 1 without advancing |
| 1, sign | `+` or `-` | advance |
| 1 | any | go to state 2 |
| 2, digits | `isdigit` true | advance |
| 2 | `isdigit` false | `K = 8`, assign the whole of `T` to `R+0x04` (`0x0099E0DE..0x0099E0E8`), exit |

If the scan reaches the terminating NUL, `K` becomes `1` and the first integer
dword `R+0x58` receives `_atol(T)` (`0x0099E0A2..0x0099E0B1`). `_atol`
(`0x009D6FDD` into `0x009D6FCC`) pushes radix `10` and calls `_strtol`
`0x009D8B23`. STATIC.

Consequences, all STATIC from the state table:

- Only the space byte counts as leading whitespace. A tab sends the scan to
  state 1 and then to the string result.
- A trailing space, a decimal point or a hex prefix gives a string result that
  keeps the entire original text, including leading spaces.
- A text of only spaces, only a sign, or a sign after spaces is accepted as the
  integer `0` because the scan ends before state 2 sees a byte.
- The scan passes the byte to `_isdigit` (`0x009D8163`) sign-extended. Behaviour
  for bytes at or above `0x80` depends on the locale table at `0x012EB4B8` or
  `__isdigit_l` and is UNKNOWN.

### Integer token conversion

`FUN_00988A30` converts each `Int` token. It is the converter described in
[Sqwt markup numeric font properties](sqwt-markup-font-numeric-properties.md).
In summary a leading `@` is skipped. Otherwise, unless the text starts with
`0x` (`0x01077264`), leading `0` characters are skipped. The rest goes to
`_strtol` with radix `0`. It does not test the end pointer. A token with no
digits, such as an unsubstituted `%s`, yields `0`. A decimal token written with
leading zeros reads as decimal because the zeros are stripped before radix
detection. A negative token with a leading `0` after the sign is read as octal
because the strip only looks at the first character. INFERRED for radix
detection and range clamping, which follow the standard `strtol` behaviour the
linked page traces.

### Limits and overflow

The integer loop has no bound against the eight slots of the record. The
count is `n - 2` for any `n`. A text with more than ten tokens writes past
`R + 0x78`. No retail value in the census below has more than two integers.
INFERRED from the loop, which compares the index only with the count.

## Reverse conversion, comparison and copy

| Routine | VA | Behaviour |
|---|---|---|
| Record to text | `0x0099DCC0` | `K == 8` formats the literal `{CommandParameter String,%s}` at `0x0107ACA0` with the string. `1 <= K <= 6` starts from `{CommandParameter Int` at `0x0107ACC0`, appends `,%d` (`0x0107ACD8`) per integer and closes with `}`. Any other `K` returns the string at `R+0x04`. |
| Equality | `0x0099DC60` | Different `K` is unequal. `K == 8` compares strings through `0x00445D20`. `K` in `1..6` compares that many integers. `K` of `0`, `7` or above `8` is equal without a comparison. |
| Copy | `0x00545780` | Copies `K`, assigns the string, and copies the eight dwords. |

STATIC. The text produced by `0x0099DCC0` for `K` in `1..6` parses back to the
same record through the token form. Integer counts of `7` and above have no
text form, and a record with `K == 8` always serializes as a string.

## Callers of the parser

The parser has six direct callers. Only the roles stated as STATIC were
confirmed in instructions.

| Caller | Source of `T` | Destination `R` | Role |
|---|---|---|---|
| `FUN_00948350`, call `0x00948520` | Raw attribute text, no evaluation | Markup object `+0xCC` | Case-insensitive `CommandParameter` attribute of an input binding markup object. The catalog row `Sqwt::Markup::InputBindingMarkupObject` slot 3 in [the vtable slot catalog](../../config/ffxivgame.vtable_slots.jsonl) names this function. STATIC. |
| `FUN_00954330`, call `0x00954405` | Result of `FUN_00916310` on the attribute value | Stack record, then compared and stored | Property value set path for a property holder at `this + 0x380`. Case 2 of `FUN_00954E70` calls it with descriptor `0x0135A244`, the `CommandParameter` property whose name literal is `0x0106E154`. Initializer `0x00F236D0` installs `FUN_00954E70` as the setter of the property class `Sqwt::DependencyPropertyInfo<Sqwt::Input::ICommandSource<Sqwt::Controls::ContentControl>>`. STATIC for the chain. The `Button` owner is INFERRED from `ContentControl`. |
| `FUN_00954500`, call `0x009545DF` | Same as above | Same, with the receiver offset by `0x18` | Sibling of the previous path for a second holder. Callers `0x00955010`, `0x009551A0` and `0x009B14B0`. Role INFERRED from the identical body. |
| `FUN_009B28C0`, call `0x009B2B1F` | Formatted text, see below | Stack record | `Sqwt::Input::ExecuteCommandAction` slot 2. Catalog row `vtable_rva_hex` `0xc7d7f4`, slot 2. STATIC. |
| `FUN_006A4ED0`, call `0x006A4F57` | The two byte literal `1`, stored at `0x006A4F1B` | Stack record | Native helper that builds the plain integer `1`, looks up a command by name through `0x00928280` and executes it. Callers include `0x008844E0` and `0x00883EC0`. STATIC. |
| `FUN_009CE7E0`, call `0x009CEC34` | String built at `0x009CEC12` from `0x009810C0` | Object `+0xFC` | Role UNKNOWN. |

Property value set paths go through `FUN_00916310` first. That routine returns
the sentinel `0x01356DA8` while a binding result is not yet available. The
callers then set bit `0x10` of the holder flag byte at `+4` and skip the
parse. Otherwise the returned string is parsed and the result is stored by
`0x00953E50` or `0x00954040`, which compare with `0x0099DC60` and copy with
`0x00545780`. A holder still pointing at the shared default `0x01361160`
gets a fresh `0x78`-byte record from `0x0052D330` first
(`0x00953EDA..0x00953EF8`). STATIC for the sequence, INFERRED for the pending
meaning of the sentinel.

### ExecuteCommandAction text substitution

`FUN_009B28C0` copies the stored template record from `this + 0x54` into a
stack record. It then builds up to two substitution strings. The two inputs
are the string members at `this + 0x80` and `this + 0x88`. Their property
descriptors `0x01361DC4` and `0x01361DE8` carry names `PropertyName1` and
`PropertyName2` at `0x0107D740` and `0x0107D750`, and getter `0x009B2BF0` and
setter `0x009B2C90` map index 0 to `+0x80` and index 1 to `+0x88`. STATIC.

For each member that is nonempty:

- If its first character is `%`, the next character minus `0x30` is passed to
  slot 1 of the event argument object in the second parameter
  (`0x009B2975..0x009B298D`). A nonnull result is copied.
- Otherwise the text names a property. `0x009108E0` finds the descriptor and
  `0x00911020` converts the property value to a string on the dependency
  object cast from the same event argument object.

If either member was nonempty, the string object whose pointer is stored at
`this + 0x78` (`0x00953A70` adds `0x78`, `0x0099A5A0` reads the dword) is used
as a `printf` format into a `0x400` byte buffer through `0x0097FD10`. The
arguments are the first substitution string and then the second
(`0x009B2AB5..0x009B2AEF`). The formatted text is parsed by
`FUN_0099DDF0` into the stack record (`0x009B2B1F`), which replaces the
template for this invocation. The `+0x78` string is the `CommandParameter`
attribute text as the property setter received it
([setter `0x00955050`](sqwt-command-parameter-consumer.md#command-objects)),
so `%s` is a C format directive resolved before the grammar above runs. If
neither member is set,
the template record parsed at load time is used as is, and a `%s` there parsed
as a token with no digits and so as `0`. INFERRED for the load-time parse,
which follows from the property path above.

## Delivery to the command dispatcher

The record reaches a command as the first argument of command slot 2.

| Step | Evidence |
|---|---|
| `FUN_009B28C0` requires a command object at `this + 0x44` and calls its slot 2 at `0x009B2B7E` with the stack record, a target and the event argument object. The target is `this + 0x4C` or, when that is null, the `Sqwt::FrameworkElement` cast of the event argument object's field `+8`, plus `0xB4`, or null. The routine then sets byte `+0x16` of the event argument object. | STATIC, read at `0x009B2B3A..0x009B2B80`. |
| Input bindings copy the markup template into the binding at `B+0x1C`. Binding slot 2 at `0x00920463` executes with that record. | Established in [the movement command origin page](../input/player-movement-command-origin.md), sections "Binding record and command arguments" (slot 2) and "Provider and configuration boundary" (the copy into `B+0x1C`). |
| The helper at `0x006A4ED0` calls the looked-up command's slot 2 with `(record, target, 0)` (`0x006A4FB3..0x006A4FC2`). | STATIC. |
| `Sqwt::Input::RoutedCommand` slot 2 `0x00927FA0` constructs the `PreviewExecuted` then `Executed` events through `0x00929100` and routes them. | Established in [the movement command origin page](../input/player-movement-command-origin.md). |

Event constructor `0x00929100` copies the record as follows. STATIC.

| Record | Event |
|---|---|
| `+0x00` kind | `+0x24` |
| `+0x04` string | `+0x28`, through `0x00447200` |
| `+0x58..+0x73`, seven dwords | `+0x7C..+0x97` |
| `+0x74`, eighth dword | `+0x98` |
| fourth argument | `+0x9C` |

The linked page names the first seven dwords. The eighth integer goes to
`+0x98`. The form-level handler that reads the kind, string and integers of an
`Executed` event and calls the script is traced in
[Sqwt CommandParameter consumer](sqwt-command-parameter-consumer.md).
The known recipients of `Executed` events are the movement handler, the form
handler and the `NumberInputBox` step handler
([consumers of the record](sqwt-command-parameter-consumer.md#other-consumers-of-the-record)).
The `Button` property consumer and every other recipient are UNKNOWN.

## Retail markup census

An in-memory SQEX decode and strict UTF-8 XML parse covered the client's
`client/sqwt` containers. The decode used `xivl-tools:tools/blowfish.py`,
whose observed SHA-256 was
`7c4208f5d3b7acf20bfb67ddcba706cbb10bf4d1c6c1677b4b11d8685774f90e`. The parse
used Python's standard-library ElementTree. A port of the grammar above
classified every `CommandParameter` attribute value. No decoded document and
no `Name` value is reproduced here.

Of 1,155 SQEX containers, 1,153 parse as XML and 105 carry the attribute, for
385 values in total. The two containers that are not strict XML are not
counted.

| Owner element | Brace, one integer | Brace, two integers | Brace, string | Binding expression | Plain integer |
|---|---:|---:|---:|---:|---:|
| `ExecuteCommandAction` | 73 | 79 | 4 | 147 | 0 |
| `Button` | 28 | 0 | 0 | 0 | 8 |
| `KeyBinding` | 14 | 0 | 0 | 0 | 18 |
| `PadBinding` | 10 | 0 | 0 | 0 | 4 |

Observations about the syntax classes:

- Every brace value spells the type as `Int` or `String`. No value uses `None`,
  a lowercase type, a hex or signed integer, an `@` prefix, or more than two
  integers.
- Brace values on `Button`, `KeyBinding` and `PadBinding` carry literal
  integers with a comma and no space after the type. The 82 literal integer
  tokens in plain and brace values are decimal in the range `1..15`, with no
  sign and no leading zero.
- Every brace value on `ExecuteCommandAction` contains one or two `%s`
  placeholders. The `Int` values place a space after each comma, except one
  two-placeholder value. The four `String` values place `%s` directly after
  the comma with no space.
- The 147 `{Binding ...}` values are all on `ExecuteCommandAction`. They are
  not accepted by `FUN_0099DDF0` directly because `{Bi` fails the `{Co` gate.
  Their key sets are `Path` with `RelativeSource` (102), `Mode`, `Path` and
  `RelativeSource` (41), and `ElementName`, `Mode` and `Path` (4). How a
  `Binding` expression becomes `T` for the property path is UNKNOWN.
- No attribute value is empty.

Per-container counts for the widget files that carry the attribute:

| Container | Owner | Class | Count |
|---|---|---|---:|
| `widget/ActionMenuWidget.form` | `Button` | brace, one integer | 10 |
| `widget/ActionMenuWidget.tpl` | `ExecuteCommandAction` | binding expression | 17 |
| `widget/ItemListWidget.tpl` | `ExecuteCommandAction` | brace, two integers | 4 |
| `widget/ItemListWidget.tpl` | `ExecuteCommandAction` | binding expression | 2 |
| `widget/LogWidget.form` | `KeyBinding` | plain integer | 4 |
| `widget/LogWidget.form` | `PadBinding` | plain integer | 2 |
| `widget/LogWidget.tpl` | `ExecuteCommandAction` | brace, string | 2 |
| `widget/PartyParameterWidget.tpl` | `ExecuteCommandAction` | brace, one integer | 1 |
| `widget/StatusEffectWidget.tpl` | `ExecuteCommandAction` | binding expression | 12 |

The table lists the widget containers named in it and is not the full set of
105 containers that carry the attribute. The `.sd` variants and the `_c`
trees are excluded from it. The census counts attribute occurrences and does
not follow `StaticResource` references. Statistics are bounded to the
installed client that was decoded. They do not establish how a value behaves
at run time.

## Reproduction

1. Verify the executable size and SHA-256 above.
2. Map the PE32 sections read-only and disassemble `0x0099DDF0..0x0099E0EE`
   with Capstone in x86 32-bit mode. Confirm the gate at `0x0099DE3D..0x0099DE66`,
   the `String` search, the tokenizer call at `0x0099DF7F`, the type tests at
   `0x0099DFDA` and `0x0099E022`, and the plain scan at `0x0099E04E`.
3. Read the data literals at `0x0107AC74..0x0107ACDC` from the section
   mapping. Disassemble `0x0044F770`, `0x00446B10`, `0x00988A30`,
   `0x0099DC60`, `0x0099DCC0`, `0x00545780` and `0x0052D330`.
4. Run `ghidra x-ref to 0x0099DDF0 --json` to list the six callers. Treat
   the decompiler output as a lead and confirm each call setup in the
   disassembly.
5. For the markup census, decode the SQEX containers in memory with the cipher
   named above. The key is the file's own base name including its suffix, as
   in `xivl-tools:docs/formats/sqex.md`, so a stem alone yields undecodable
   bytes. Parse the result as XML and
   classify each `CommandParameter` attribute with a port of the grammar. Keep client bytes, decoded documents
   and executable listings outside the repository.

## Surviving gaps

- How a `{Binding ...}` expression is evaluated into the text `T` that the
  property paths parse.
- Which command consumers other than the movement handler and the form-level
  script handler read the kind, string or integers from an `Executed` event,
  and the consumer of the `Button` property record.
- The role of the call at `0x009CEC34`.
- Behaviour of `_isdigit` for bytes at or above `0x80`.
- Whether the type name `Sqwt::Input::CommandParameter`, present as the type
  descriptor string `.?AUCommandParameter@Input@Sqwt@@` at `0x012E3040`, names
  this record. The string is present. A join from the record to that type is
  not observed.
