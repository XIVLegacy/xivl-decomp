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
below with Ghidra 12.1.4 through ghidra-cli. The sheet cells come from the
[text command sheet rows](https://github.com/XIVLegacy/xivl-client-data/blob/main/docs/text-command-rows.md).

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

## Command lookup

`0x0056D3C0` opens the sheet named by the literal `xtx/_textCommand`
(`0x00FA5D60`) and returns `0xFFFFFFFE` when the sheet is absent or no row
matches. It asks the sheet for a row by column 5 (vtable slot `+0x14`) and
then retries with the two alternate columns that slot `+0x20` returns for 0
and 1. The parser treats a returned value that is negative as a 16-bit
signed number as a miss (`0x0056E6A4..0x0056E6AA`). A hit returns the row key.
STATIC.

## Row accessor

`0x00C9A480(n)` returns `*(object+8) + 4 * byte[*(*(object+0xC)+0x14) + n]`,
a cell pointer chosen through the row's key-to-cell byte table. The parser
reads these keys of the matched row:

| Key `n` | Use | Locator |
| ---: | --- | --- |
| 0 | Token budget, a byte | `0x0056E77D..0x0056E788` |
| 1 | Numeric word count, a byte, stored as the word at output `+0x02` | `0x0056E6DE..0x0056E6EF` |
| `n+2` | Resolver kind of numeric word `n`, a signed byte | `0x0056E719..0x0056E72E` |

Column meaning in the installed sheet is in the
[text command sheet rows](https://github.com/XIVLegacy/xivl-client-data/blob/main/docs/text-command-rows.md).
That page reads keys 0 to 4 as columns 0 to 4. INFERRED for the key to column
equality, as in the cell table of any sheet that stores its keys in order.

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

A flag is set at `0x0056E6BA..0x0056E6DE` when the object at `this+4` answers 4
or 5 through its vtable slot `+0x18` and the key is `0x67`, the `/tell` row of
the installed sheet. With the flag set the loop pushes the literal `!!!`
(`0x00FA5D90`) as an extra string after each token that does not begin with
`<` (`0x0056E835..0x0056E864`). STATIC for the push. The meaning of the extra
string and of states 4 and 5 are untraced.

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
base, the case rule of text compare, and the bodies of `0x0056D4D0`,
`0x0056D720` and `0x0056E9C0` are untraced.

## Lookup tables

The static initializer `0x00F0C930` binds two sheet readers. It stores the
literal `xtx/command` (`0x00FA5D28`) at `0x01338078` with the key ranges
`0x69DC..0x6B08`, `0x733C..0x74CB` and `0x5661..0x5662` as six dword stores at
`0x013380CC..0x013380DC`. It stores the literal `xtx/_subCommand`
(`0x00FA5D18`) at `0x013380E8` with six zero dwords at
`0x0133813C..0x0133814C`. STATIC for the stores. INFERRED that the kinds 2 and
above and below -1 select these two tables, since the caller chain from the
resolver to the readers was read only to `0x0056D4D0` and `0x0056D720`.

## Reproduction

Locate the literals with `ghidra strings list -f 'value~Command'` and walk the
cross-references. Disassemble `0x0056E380`, `0x0056E040`, `0x0056DAA0`,
`0x0056D3C0` and `0x00F0C930` and match each locator above. Read the sheet
cells from an extraction of the installed client with the recipe of the
linked sheet page.

## Boundary

The pre-check object, the `/tell` extra string and its states, the trailing
cut of the working copy near `0x0056E4F0`, the number rule of kind 0, the
bodies behind kinds 2 and above and the item form, and the case rule of text
compare are open. No packet capture exercises a text command.
