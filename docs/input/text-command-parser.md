# Text command parser core

The chat line parser `0x0056E380` turns a submitted line into a command key,
up to three resolved numeric words and a list of text words. The Lua native
`_parseTextCommand` calls it and marshals the result, as recorded in
`xivl-client-structs:docs/desktop-state-native-contract.md`, "Text command and
countdown". That page names the call and the record fields and leaves the
grammar untraced. This page records the grammar that the instructions show.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. The
observations come from disassembly and decompilation of the functions named
below with Ghidra 12.1.4 through ghidra-cli. Table and string reads were taken
from the section-mapped bytes of the same PE. The sheet cells come from the
[text command sheet rows](https://github.com/XIVLegacy/xivl-client-data/blob/main/docs/text-command-rows.md).
The [lookup sheet definitions](#lookup-sheet-definitions) come from two
decoded definition documents of the installed client.

## Entry and result

`0x0056E380` is `__thiscall`. Its stack arguments are the line and the output
record, and it ends with `RET 8`. The output record holds a word at `+0x00`
(the command key), a word at `+0x02` (the numeric word count), three dwords
from `+0x04` and a vector of `Utf8String` values at `+0x10`. STATIC.

| Result | Locators |
| --- | --- |
| Returns 1 | `MOV AL,1` at `0x0056E5BD`, reached from `0x0056E592` |
| Returns 0 | `XOR AL,AL` at `0x0056E96D`, reached from `0x0056E942` |

The false answer is reached by three branches to `0x0056E942`:

- The pre-check object at `this+8` answers zero. The call is at
  `0x0056E3FE..0x0056E41F` through its vtable slots `+0x04` and `+0x08` and the
  test and branch are at `0x0056E421..0x0056E423`. What the object tests is
  untraced.
- The first byte of the working copy is `0x20` (`0x0056E5D1..0x0056E5D3`).
- The splitter `0x0044F840` yields no token, or the first token does not
  begin with `/` (`0x0056E64D..0x0056E687`, target `0x0056E931`).

Two true answers store `0xFFFF` in the word at `+0x00` and 0 in the word at
`+0x02`, so the script reads the key as -1:

- An empty working copy stores both words at `0x0056E54E..0x0056E553` and
  pushes one empty string at `0x0056E557..0x0056E57C`.
- A command lookup miss stores both words at `0x0056E90D..0x0056E912` and
  pushes the whole original line at `0x0056E916`.

## Sheet provider

`0x0075CCF0` loads a pointer from its receiver, passes it to `0x004D7520`,
which adds `0x8BC`, and tail-jumps to `0x0056E380` with the result. At
`0x004DC0AF..0x004DC0B7` the container constructor `0x004DBF40` builds an
object at its own `+0x8BC` through `0x0056D2C0`. The first stack argument is
stored at object `+4` (`0x0056D2ED..0x0056D300`). It is the register value
that `0x004DC065` passes to the `TextModule` constructor, which the
[installed provider finding](../resource/logcontrol-text-lookup.md#installed-provider)
identifies as the `ExcelModule`. STATIC.

INFERRED. The parser's receiver is that object, so its `+4` is the
`ExcelModule`. The unproved link is that the pointer `0x0075CCF0` loads, and
the receiver of the [column load call](#lookup-column), are the container that
`0x004DBF40` constructs. The stores that supply those two pointers are
untraced. The parser's use of provider slots `+4` and `+0x18` agrees with the
module table below. The sheet behaviour on this page rests on this join.

The [named source result](player-movement-command-origin.md#named-source-result-and-row-blocks)
owns that module's table. Slot `+4` (`0x00C981D0`) returns the `ExcelSheet`
registered under a name. Slot `+0x18` (`0x00C98150`) returns the address of
the module's language field at `+0x14`. The sheets use table `0x0110C6D4`.
The lookups on this page use the sheet slots below. STATIC for the table
targets.

| ExcelSheet slot | Target | Behaviour |
| ---: | --- | --- |
| `+0x10` | `0x00CAF170` | Row count from the cache when sheet `+0x3C` is zero |
| `+0x14` | `0x00CAF210` | `(column, text, key output)`. Walks rows in cache index order and returns the first ready row whose cell in that logical column equals the text. Returns null when `+0x3C` is nonzero |
| `+0x1C` | `0x00CAF300` | Returns the byte at sheet `+0x0B`, the [default logical column](../resource/logcontrol-text-lookup.md#default-column-configuration) |
| `+0x20` | `0x00CAF310` | `(n)` returns byte `n` of the array at sheet `+0x0C`, with no bounds check |
| `+0x30` | `0x00CAF2A0` | `(index, key output)` returns the ready row at a cache index |

The array behind slot `+0x20` comes from the `TABLE` entry of the sheet's
`param` attribute. The
[default column finding](../resource/logcontrol-text-lookup.md#default-column-configuration)
owns the split of that attribute into name and value pairs and the `SHEET`
query. The same override `0x00CAFA80` then queries the name `TABLE`
(`0x00CAFE53..0x00CAFEA5`, literals `0x0110C560` and `0x0110C55C`), splits its
value on `;`, allocates one byte for each part and stores that array at sheet
`+0x0C` (`0x00CAFEF5..0x00CAFF00`). Each part is converted in base 10 and its
low byte is stored in order (`0x00CAFF2D..0x00CAFF38`). The constructor first
clears `+0x0C` at `0x00CB0500`. STATIC.

## Command lookup

`0x0056D3C0` opens the sheet named by the literal `xtx/_textCommand`
(`0x00FA5D60`) through the provider's slot `+4`. It returns `0xFFFFFFFE` when
the sheet is absent or no row matches. It calls sheet slot `+0x14` up to three
times, each a complete walk of the rows. The first call uses the literal
logical column 5 (`0x0056D439`). After a miss it uses the column that slot
`+0x20` returns for 0 (`0x0056D443..0x0056D45A`), and after a second miss the
column for 1 (`0x0056D460..0x0056D477`). The cell compare at `0x004454E0` is
an exact comparison of length and bytes. The parser treats a returned value
that is negative as a 16-bit signed number as a miss
(`0x0056E6A4..0x0056E6AA`). A hit returns the row key. STATIC.

The two alternate columns are therefore `TABLE` entries 0 and 1 of the loaded
sheet. The [definitions](#lookup-sheet-definitions) give them for each
language.

## Row accessor

`0x00C9A480(n)` returns `*(object+8) + 4 * byte[*(*(object+0xC)+0x14) + n]`,
a cell pointer chosen through the row's column-to-cell byte table. The
[default column finding](../resource/logcontrol-text-lookup.md#default-column-configuration)
shows that this table maps each logical column of the definition's index list
to its position in that list. `n` is therefore a logical column. The parser
reads these columns of the matched row:

| Column `n` | Use | Locator |
| ---: | --- | --- |
| 0 | Token budget, a byte | `0x0056E77D..0x0056E788` |
| 1 | Numeric word count, a byte, stored as the word at output `+0x02` | `0x0056E6DE..0x0056E6EF` |
| `n+2` | Resolver kind of numeric word `n`, a signed byte | `0x0056E719..0x0056E72E` |

Every installed `xtx/_textCommand` definition lists columns 0 to 5 first and
in order. Column meaning in the installed sheet is in the
[text command sheet rows](https://github.com/XIVLegacy/xivl-client-data/blob/main/docs/text-command-rows.md).
STATIC.

## Numeric word loop

The loop at `0x0056E6F3..0x0056E77B` runs three times (`CMP EDI,3` at
`0x0056E778`). Word `n` is resolved from token `n+1` through `0x0056E040` when
`n` is below the word count stored at output `+0x02` and the token exists
(`0x0056E70A..0x0056E717`). Otherwise the dword is `0xFFFFFFFF`
(`0x0056E769`). The loop stores each result at output `+0x04 + 4*n`.

## Text word loop

`0x0056E77D..0x0056E904` pushes the text words. The budget byte bounds the
loop. Token 0 is the command word and is not pushed. The loop advances an
offset by the tokenizer's consumed length, pushes tokens 1 up to the budget
less one, and spends one extra budget slot on a token whose first byte is `<`
(`0x0056E81C..0x0056E875`). When a token remains after the budget is spent,
the rest of the line from the consumed offset is pushed as one string through
`0x00447A40` (`0x0056E8B9..0x0056E904`).

A flag is set at `0x0056E6B0..0x0056E6D9` when the provider's language field,
read through slot `+0x18`, is 4 or 5 and the key is `0x67`, the `/tell` row of
the installed sheet. These are the codes of `chs` and `cht`. With the flag set
the loop pushes the literal `!!!` (`0x00FA5D90`) as an extra string after each
token that does not begin with `<` (`0x0056E835..0x0056E864`). STATIC for the
push. The meaning of the extra string is untraced.

The splitter `0x0044F840` is called with the separator literal `" ` (a quote
and a space, `0x00FA5D8C`) and fills a vector of 84-byte `Utf8String` tokens
(`0x0056E641`, the divide by `0x54` at `0x0056E65B`).

## Tokenizer

`0x0056DAA0` returns the consumed length and builds one token. STATIC:

- A byte `0x02` begins a marked payload. The tokenizer copies `2 + byte[2]`
  bytes and continues (`0x0056DAC8..0x0056DAD6`).
- A space ends the token when the token is non-empty. A leading space is
  skipped (`0x0056DADE..0x0056DAEB`).
- A `"` opens a quoted span only when the token is still empty. The span runs
  to the next `"` or to the end of the text, and the closing quote is consumed
  (`0x0056DB06..0x0056DB43`). A `"` after a non-empty token ends that token.

## Resolver

`0x0056E040(token, kind, error)` dispatches on the signed kind byte (`CMP
EDI,0` and `CMP EDI,1` at `0x0056E092` and `0x0056E0C2`):

| Kind | Route |
| --- | --- |
| 0 | Number: `0x004553A0` then `0x004555A0` with base 10 (`0x0056E09E..0x0056E0B5`) |
| 1 | Command word: a token that lacks a leading `/` is joined to the literal `/` (`0x00FA5D74`) and looked up through `0x0056D3C0` (`0x0056E0C5..0x0056E14B`) |
| 2 and above | Table lookup `0x0056D6E0(token, kind - 2)`, which calls `0x0056D4D0` (`0x0056E2CC..0x0056E2D1`) |
| -1 | Item form: split by the literal `+` (`0x00FA5D78`), with calls to `0x004555A0`, `0x0056E9C0` and `0x0056D720` (`0x0056E15D..0x0056E267`) |
| below -1 | Table lookup `0x0056D720(token, -1 - kind)` (`0x0056E2BC..0x0056E2C5`) |

The installed rows use kinds -1, 0, 1, 2 and 3. The number rule beyond the
base, any case change made to a token before a compare, and the bodies of
`0x0056D720` and `0x0056E9C0` are untraced.

## Lookup tables

The static initializer `0x00F0C930` binds two sheet readers, `0x70` bytes
apart. It stores the literal `xtx/command` (`0x00FA5D28`) at `0x01338078`
with the key ranges `0x69DC..0x6B08`, `0x733C..0x74CB` and `0x5661..0x5662`
as six dword stores at `0x013380CC..0x013380DC`. It stores the literal
`xtx/_subCommand` (`0x00FA5D18`) at `0x013380E8` with six zero dwords at
`0x0133813C..0x0133814C`. For both readers it stores 0 in the column byte at
reader `+0x6C` and 1 in the byte at reader `+0x6D`
(`0x00F0C9A8..0x00F0C9AE` and `0x00F0C9FC..0x00F0CA02`). STATIC.

`0x0056E2CC..0x0056E2D1` passes the kind minus 2 to `0x0056D6E0`. That
function uses the value as an index into the sheet pointers at `0x01337CF4`
and, times `0x70`, into the readers. Kind 2 selects `xtx/command` and kind 3
selects `xtx/_subCommand`. It calls `0x0056D4D0` once with that sheet, reader
and token. It returns the matched row's key, or `0xFFFFFFFE` when the sheet
pointer is null or no row matches. STATIC.

## Lookup column

The zero in the column byte is not the final value. `0x0056D890` walks the two
slots. For each null sheet pointer it opens the sheet by the reader's name
through the provider's slot `+4` (`0x0056D8CC..0x0056D8DC`). When the sheet is
nonnull it calls sheet slot `+0x1C` and stores `AL` in that reader's `+0x6C`
(`0x0056D8E0..0x0056D8E9`). The store address is the register that starts at
`0x013380E4` (`0x0056D8BD`) and advances by `0x70` (`0x0056D8EE`), so a
direct reference search for the second byte at `0x01338154` shows only the
static write. The call at `0x004DA7C1` passes its receiver plus `0x8BC`. The
meaning of the local byte that gates that call is untraced. STATIC.

## Table lookup compare

`0x0056D4D0` walks row indices from 0 to the sheet's row count and returns the
first match. STATIC:

- It fetches each row and its key through sheet slot `+0x30`
  (`0x0056D576..0x0056D57F`).
- It applies the three key ranges at reader `+0x54..+0x68`, each a begin and
  end pair with an inclusive match (`0x0056D58B..0x0056D5C3`). A zero end in
  the first pair admits the row. A zero end in the second pair skips to the
  third pair. A zero end in the third pair, when reached, rejects the row. A
  nonzero pair that does not contain the key falls through to the next pair,
  and a miss on the third rejects the row. All of the `xtx/_subCommand` ends
  are zero, so its first pair admits every row.
- It requires a ready row entry (`0x0056D5DF..0x0056D5E6`).
- It reads one cell, the column in the byte at reader `+0x6C`. Each row has
  one `0x00C9A480` call and one compare on either branch. It has no retry on
  another column and does not use sheet slot `+0x20`.
- When reader `+0x6D` is 1, it copies the cell string, passes the copy through
  the text evaluator `0x00C99E80` and compares the result with the token
  through `0x00445D20` (`0x0056D5F6..0x0056D653`). Otherwise it compares the
  cell with the token through `0x004454E0` (`0x0056D67B..0x0056D695`). Both
  compares require equal length and equal bytes. What the evaluator changes
  is untraced.

## Lookup sheet definitions

The [named source result](player-movement-command-origin.md#named-source-result-and-row-blocks)
owns the master selection. The game master `0x01030000` names the definition
resources of these sheets through `infofile`: `189073651` for
`xtx/_textCommand` and `189071519` for `xtx/_subCommand`. Both documents decode
with `decode_document` from
[xivl-tools at fd4c97d](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/tools/research/census_sheet_stack.py).

| Definition | Carrier bytes | Carrier SHA-256 | Decoded bytes | Decoded SHA-256 |
| --- | ---: | --- | ---: | --- |
| `189073651` | 3367 | ed438fe3babe55d5f9e57ac00a7e3ccb72f3453f1b49d658a0d51928f7f131d2 | 3366 | 0faab144d8250b32fa377b8aff31a0541c0e677c535719d4686965ab0416e12d |
| `189071519` | 2321 | 2a87b4280103bc26fe9ecb0eccd830cb0571c956e2e308afe36cb2b4f1638bbc | 2320 | 7269c7902e0b292a149427679b7eb32f5ef0011bb130509197fa210dc64588ff |

Each document holds one `client` sheet for each of five languages. The
registration `0x00C98580` admits a sheet with a nonempty `lang` only when the
code that `0x00D34D20` returns for it equals the module's language field
(`0x00C98684..0x00C986A1`). The codes are `ja` 0, `en` 1, `de` 2, `fr` 3,
`chs` 4 and `cht` 5, with `-1` for another string. STATIC.

Every `xtx/_textCommand` entry has a `param` with a `TABLE` pair and no `SHEET`
pair. Every `xtx/_subCommand` entry has one `str` column, one declared logical
index and a `param` with neither a `TABLE` nor a `SHEET` pair. STATIC.

| `lang` | `_textCommand` `TABLE` columns | Command word columns in lookup order | `_subCommand` declared and compared column |
| --- | --- | --- | ---: |
| `ja` | 6, 7 | 5, 6, 7 | 0 |
| `en` | 9, 10 | 5, 9, 10 | 1 |
| `de` | 12, 13 | 5, 12, 13 | 2 |
| `fr` | 15, 16 | 5, 15, 16 | 3 |
| `chs` | 18, 19 | 5, 18, 19 | 4 |

The `TABLE` columns are STATIC from the definitions and the override above.

The declared index in the last column is STATIC. That it is also the compared
column is INFERRED. The constructor stores the one declared index at
`ExcelSheet +0x0B` (`0x00CB0811`). The override changes that byte only for a
`SHEET` pair named `DEF`, which these entries lack. `0x0056D890` copies the
byte into the reader. A scan of the disassembly at `0x00CAF000..0x00CB1279`
and `0x00CC2000..0x00CC420F` for byte stores at register offset `+0x0B` found
only `0x00CAFE14`, `0x00CB04FC` and `0x00CB0811`. The unproved link is that
`+0x0B` still holds the stored index when `0x0056D890` reads it. The scan
excludes neither wider or computed stores nor code outside those spans, and
the [callback paths during sheet
construction](../script/logcontrol-player-producer.md#native-default-and-execution-boundary)
are not closed. With that link and the [provider join](#sheet-provider), a
sub-command word is compared with the cell in the loaded language's column.
That each row is compared on one cell only is STATIC in [table lookup
compare](#table-lookup-compare) and does not depend on either link.

The language that a running client selects is UNKNOWN. Module initialization
`0x00C97FC0` copies the dword behind its third stack argument into the language
field (`0x00C97FDB..0x00C97FF2`). The startup call at `0x004B358F` passes the
address of startup receiver `+0x38` (`0x004B351F` and `0x004B3584`). The
producer of that value is untraced. No definition response or sheet admission
was observed in a running client.

## Reproduction

Locate the literals with `ghidra strings list -f 'value~Command'` and walk the
cross-references. Disassemble `0x0056E380`, `0x0056E040`, `0x0056DAA0`,
`0x0056D3C0`, `0x0056D4D0`, `0x0056D6E0`, `0x0056D890`, `0x00F0C930`,
`0x0056D2C0` and `0x0075CCF0` and match each locator above. Read the container
site at `0x004DC058..0x004DC0B7` and the load call at `0x004DA7C1`. Read the
`ExcelSheet` table at `0x0110C6D4` and its slot targets, then follow the
override `0x00CAFA80`, the registration `0x00C98580`, the language code
function `0x00D34D20` with its literals at `0x01110654..0x01110668`, and the
module initialization and startup call. Decode the two definition resources
from an installed client, compare their hashes and read the `lang`, `param`
and index values. Read the sheet cells from an extraction of the installed
client with the recipe of the linked sheet page.

## Boundary

The pre-check object, the `/tell` extra string, the trailing cut of the working
copy near `0x0056E4F0`, the number rule of kind 0, the bodies behind kinds
below -1 and the item form, the effect of the text evaluator on a table cell,
and any case change made to a token before a compare are open. The
[provider join](#sheet-provider) is INFERRED. The retention of the default
column byte and the runtime language are bounded in
[lookup sheet definitions](#lookup-sheet-definitions). No packet capture
exercises a text command.
