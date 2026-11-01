# Rich-text sheet and case handlers

The rich-string formatter `FUN_00CB2760` expands the `0x02` tokens of a text
row through a handler vector. This page records which token ids have a
handler, how the six integer comparison leads order their operands, and what
three handlers do: `0x28` reads a cell of a named sheet, `0x2B` capitalizes
the first letter of a string and `0x2D` capitalizes every word. The handlers
`0x08`, `0x09`, `0x20` and `0x29` stay with
[Sqwt plain text reference lookup](sqwt-plain-text-reference-lookup.md),
SQTP-04 and SQTP-06, and
[Rich-string entry count controls](rich-string-entry-counts.md). The colour
handlers are in [Rich-text colour tokens](rich-text-colour-tokens.md).

This page does not establish the failure return of a handler, a sheet other
than the ones named below, non-integer comparisons, or any rendered text.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | `ghidra-cli 0.2.2` disassembly, decompilation as a lead and memory reads of vtables and literals |

All addresses are VA. Subtract `0x00400000` for RVA. Executable size and
SHA-256 were checked first. Every instruction-level claim was read from the
disassembly and every table cell from the executable's data. This was static
analysis, without client execution.

Evidence labels:

- STATIC: read directly from instructions or data at the cited address.
- INFERRED: a reading that joins static facts with a convention. The join is
  stated.
- UNKNOWN: not established.

Installed-data observations are marked as such. They give token ids, operand
kinds and counts from the installed `xtx` sheets and carry no row text.

## RT-01: Handler vector

The TextModule constructor `FUN_00CA0390` (`0x00CA0390..0x00CA0898`) stores
thirty handler thunks into the vector at decoder `+8`, each after a bounds
guard against the vector size. Each thunk is two instructions,
`MOV EAX,[ECX]` and `JMP [EAX+offset]`. The decoder vtable is `0x01108ECC`
(SQTP-04), so the handler is the dword at `0x01108ECC` plus the offset.
STATIC.

| Code | Thunk | Offset | Handler |
|---|---|---|---|
| `0x07` | `0x00B24200` | `+0x04` | `0x00C9EFC0` |
| `0x08` | `0x00A53E30` | `+0x08` | `0x00C9F000` |
| `0x09` | `0x00B24230` | `+0x0C` | `0x00C9F140` |
| `0x11` | `0x00A53E20` | `+0x14` | `0x00C9F1C0` |
| `0x12` | `0x00CA1600` | `+0x18` | `0x00C9F1E0` |
| `0x13` | `0x00B241D0` | `+0x20` | `0x00C9F270` |
| `0x14` | `0x00B24210` | `+0x24` | `0x00C9F2C0` |
| `0x15` | `0x0059BD10` | `+0x28` | `0x00C9F310` |
| `0x18` | `0x00B24220` | `+0x34` | `0x00C9F330` |
| `0x1E` | `0x00B24240` | `+0x1C` | `0x00C9F200` |
| `0x20` | `0x00A4B0F0` | `+0x50` | `0x00C9F350` |
| `0x21` | `0x008F70A0` | `+0x54` | `0x00C9F440` |
| `0x22` | `0x008F70C0` | `+0x58` | `0x00C9F530` |
| `0x23` | `0x008F7080` | `+0x5C` | `0x00C9F670` |
| `0x24` | `0x00A4B100` | `+0x60` | `0x00C9F760` |
| `0x25` | `0x008E6320` | `+0x64` | `0x00C9F850` |
| `0x26` | `0x008F70D0` | `+0x68` | `0x00C9F990` |
| `0x28` | `0x00CA1590` | `+0x6C` | `0x00CA0E20` |
| `0x29` | `0x00CA15B0` | `+0x70` | `0x00C9FAB0` |
| `0x2A` | `0x00CA15D0` | `+0x74` | `0x00C9FB60` |
| `0x2B` | `0x00A4B120` | `+0x78` | `0x00C9FC20` |
| `0x2C` | `0x00A4B0D0` | `+0x7C` | `0x00CA01C0` |
| `0x2D` | `0x00A4B110` | `+0x80` | `0x00C9FD50` |
| `0x2E` | `0x00CA15E0` | `+0x84` | `0x00C9FEF0` |
| `0x2F` | `0x00A4B130` | `+0x88` | `0x00C9FE30` |
| `0x30` | `0x00A4B0E0` | `+0x8C` | `0x00CA1340` |
| `0x31` | `0x00CA15C0` | `+0x90` | `0x00CA1370` |
| `0x32` | `0x00CA15F0` | `+0x94` | `0x00CA13A0` |
| `0x33` | `0x00CA1610` | `+0x98` | `0x00CA13D0` |
| `0x34` | `0x00CA15A0` | `+0x9C` | `0x00CA1400` |

The vector holds `0x37` zero-initialized entries (the entry count page), so
every other code below `0x37` has a null entry. The ids `0x10`, `0x16`, `0x19`,
`0x1A` and `0x1D` occur in installed rows and are among them.

For a null entry, `FUN_00CB2760` at `0x00CB27FD..0x00CB2813` decodes the
length expression with `FUN_00CB2D50` and copies the token whole to the output.
The copied length is the length width plus the declared payload length plus 3
(`0x00CB281C`), counted from the `0x02` byte. The output therefore carries
`0x02`, the code, the length, the payload and `0x03` unchanged. STATIC.

INFERRED. A later consumer of the formatted bytes interprets such a token. A
text row that holds one of those ids is not plain text after formatting.
[Rich-text colour tokens](rich-text-colour-tokens.md) records one such consumer.

## RT-02: Integer comparison leads

`FUN_00CB2D50` takes a lead from `0xE0` to `0xE5` at `0x00CB2E86` (`CMP AL,0xE0`)
and decodes two operands after it. `FUN_00CB2560` gives the string kind to the
leads `0xEA`, `0xEB` and `0xFF` and the integer kind to every other lead, and
the operand is then read by `FUN_00CB2AE0` or `FUN_00CB2D50`. The first
operand is stored at stack offset `+0x1C` and the second at `+0x20` of the
frame base at the join `0x00CB2F73..0x00CB2F7B`. The jump at `0x00CB2F8E`
indexes the dword table `0x00CB3218` by the lead minus `0xE0`.
STATIC.

When both operands are integers, `CMP EAX,ECX` compares the first operand
(`EAX`) with the second (`ECX`), and the flag byte is written to the output
dword at `0x00CB3071`.

| Lead | Table cell | Target | Condition | Result is 1 when |
|---|---|---|---|---|
| `0xE0` | `0x00CB3218` | `0x00CB2F95` | `SETGE` at `0x00CB2F9D` | first >= second |
| `0xE1` | `0x00CB321C` | `0x00CB2FBD` | `SETG` at `0x00CB2FC5` | first > second |
| `0xE2` | `0x00CB3220` | `0x00CB2FE5` | `SETLE` at `0x00CB2FED` | first <= second |
| `0xE3` | `0x00CB3224` | `0x00CB3007` | `SETL` at `0x00CB300F` | first < second |
| `0xE4` | `0x00CB3228` | `0x00CB3029` | `SETZ` at `0x00CB3031` | first == second |
| `0xE5` | `0x00CB322C` | `0x00CB304B` | `SETNZ` at `0x00CB3053` | first != second |

The comparisons are signed. Operand kinds that differ are converted before the
compare (`0x00CB2F3B..0x00CB2F6D`). String operands call other comparison
helpers (`0x00446EC0`, `0x00446F20`, `0x00446E90`, `0x00446EF0`, `0x00445D20`,
`0x00445DB0`). Those paths are not traced here. UNKNOWN.

The supplied-argument lead `0xE8` and the one-based index rule are SQTP-04. An
`0xE8` over a literal is a legal operand of every lead in the table.

## RT-03: Handler 0x28, named-sheet text

Handler `FUN_00CA0E20` (`0x00CA0E20..0x00CA0FF3`) decodes, in payload order:

1. A string expression with `FUN_00CB2AE0` in mode 1 at `0x00CA0E95`. It is the
   sheet name.
2. An integer expression with `FUN_00CB2D50` at `0x00CA0EB1`. It is the row.
3. An integer expression with `FUN_00CB2D50` at `0x00CA0ECE`. It is the field.
4. Every remaining payload byte through `FUN_00CB3480` at `0x00CA0F0C`. That
   routine loops while the cursor is below the payload end. It decodes each
   expression as an integer with mode 0 and, when that consumes nothing, as a
   string with `FUN_00CB2AE0` mode 0. It appends one tag 0 or tag 1 variant per
   expression to a local container. An empty remainder appends nothing.

It then calls virtual `+0x0C` of the outer TextModule (`[EBP-4]`, the table
`0x01108F70`) at `0x00CA0F4C` with the name, row, field, that container and a
local output string. The target is `FUN_00CA0930`. On a return of 1
(`0x00CA0F4E`) it appends the local output to its own output with
`FUN_00447C80` at `0x00CA0F5F` and returns the accumulated expression width at
`0x00CA0F9D`. STATIC.

`FUN_00CA0930` is the lookup SQTP, Concrete lookup and loaded-schema condition,
describes. The table is the sheet found by name through TextModule `+0x30`
virtual `+4`, the row is read through virtual `+0x18`, and the supplied field is
a logical field that `FUN_00CAF330` maps to a physical column. A field whose
mapped type is `str` (type code `10`) goes to TextModule virtual `+8`, which
formats the cell with the container above as its argument vector. The cell does
not see the argument vector of the row that holds the `0x28` token. STATIC for
the call and INFERRED for the cell formatting, which SQTP-04 already carries.

The stack arguments of `FUN_00CA0930` are the name at `[ESP+0x418]`, the row at
`[ESP+0x41C]` and the field at `[ESP+0x420]` (`0x00CA0966..0x00CA099B`). A
negative field calls virtual `+0x20` of the sheet with the field complemented,
and a field of exactly `0xFF` calls virtual `+0x1C` (`0x00CA09A5..0x00CA09CC`).
Any other field goes to the type query directly. The two special branches are
not traced. UNKNOWN. The failure return when the sheet, row or field is
missing is also UNKNOWN.

### Installed sheet observation

The installed `xtx/text_jobName` definition (resource `0x0B450269`,
`xivl-client-data:manifests/sheet_inventory.csv`) has one definition per
language, and each lists its own logical fields in physical order:

| Language | Logical fields | Types |
|---|---|---|
| `ja` | `0` | `str` |
| `en` | `1` to `5` | `str`, `str`, `s8`, `s8`, `str` |
| `de` | `6` to `11` | six `str` |
| `fr` | `12` to `20` | nine `str` |
| `chs` | `21` | `str` |

The field literal of each `0x28` token in `xtx/_text_ui` row 231 names a field
of the definition for the language of that row's column: `0`, `1`, `6` and `16`
for `ja`, `en`, `de` and `fr`. Installed data observation, STATIC for the
schema.

Across the whole installed `en` table the two most frequent `0x28` operand
shapes are a literal sheet name, the supplied integer 1 and a literal field (63
of 164 tokens, for the fields 1 and 2), and a literal name, a nested `0x28`
token as the row and a literal field (21 tokens). The second shape puts a string
expression where the row decoder expects an integer. `FUN_00CB2D50` mode 1 then
decodes it as a string and converts it with `FUN_004555A0` (`0x00CB315A..0x00CB31B0`).
That conversion is not traced. UNKNOWN.

## RT-04: Handlers 0x2B and 0x2D, capitalization

Both handlers decode one string expression with `FUN_00CB2AE0` mode 1
(`0x00C9FC8C` and `0x00C9FDB5`), call `FUN_00445220` on the decoded string, and
append the string with `FUN_00447C80`. A zero decode width appends nothing.

Handler `0x2B` first skips tokens at the head of the decoded string. While the
byte is `0x02` it decodes the length with `FUN_00CB2D50` and steps over
`0x02`, the code, the length and `0x03` (`0x00C9FCA6..0x00C9FCD2`). The count
of skipped bytes is the start index. STATIC.

Both handlers read the language through TextModule decoder `+0x2C`, virtual
`+0x18` (`0x00C9FCD4..0x00C9FCDC`, `0x00C9FDC0..0x00C9FDC8`). That target is
`FUN_00C98150`, which returns ExcelModule `+0x14`, the language number of SQTP,
Default table and named language fields. STATIC.

| Handler | Pushes before the call | Meaning |
|---|---|---|
| `0x2B` | `0x00C9FCEE..0x00C9FCF4` | word-start gating 1, all words 0, French flag set when the language is 3, exception list null, start index as computed |
| `0x2D` | `0x00C9FDE3..0x00C9FDEB` | word-start gating 1, all words 1, French flag set when the language is 3, exception list `0x01108EA4` when the language is 1 and null otherwise, start index 0 |

The pushes are in right-to-left order, so the first push is the last
parameter. The exception pointer follows from `0x00C9FDCC..0x00C9FDD7`
(`SUB`, `NEG`, `SBB`, `NOT`, `AND 0x01108EA4`), which gives the pointer for a
language of exactly 1. STATIC.

`FUN_00445220` (`0x00445220..0x004453A5`) walks the string in place. It reads
one byte per step and keeps a word-start flag that is true at the start
(`0x0044522D`) and after a space (`0x0044537C..0x0044537E`). Its first
parameter is the word-start gating and both handlers pass 1:

- The end bound is the start index plus 1 unless the all-words parameter is
  set, in which case it is the string size (`0x00445231..0x0044523B`).
- With the gating set, only a word start changes a byte
  (`0x00445250..0x00445265`).
- A byte from `a` to `z` becomes the byte minus `0x20` (`0x0044526B..0x00445271`
  and `0x004452C5..0x004452CA`). With an exception list, the byte is left alone
  when any entry matches by prefix. An entry is a pointer and a length, the list
  ends at a zero length, and the compare is `_strncmp` over the entry length
  (`0x00445273..0x004452B4`).
- A UTF-8 lead `0xC3` whose next byte is `0xA0` to `0xB6` or `0xB8` to `0xBE`
  has that next byte lowered by `0x20` (`0x004452D1..0x004452E9`,
  `0x00445396..0x0044539B`). When the French flag is set and the position is a
  word start, a next byte `0xA0` to `0xA5` replaces the lead with `A`, `0xA8`
  to `0xAB` with `E`, `0xAC` to `0xAF` with `I`, `0xB2` to `0xB6` with `O` and
  `0xB9` to `0xBC` with `U`, and the second byte is removed by shifting the tail
  left and lowering the size by 1 (`0x004452F3..0x0044536F`). Other accented
  bytes are left alone in that case.
- The lead `0xC5` followed by `0x93` becomes `0xC5 0x92` (`0x0044539D` onward).

The exception list at `0x01108EA4` holds the entries (`0x01108E78`, 3) and
(`0x01108E70`, 4), then a zero length. The strings are `of ` and `the `, each
with a trailing space (data reads at those addresses). STATIC.

The end bound of an all-words walk is read once, before the loop. After a
French accent removal the size shrinks and the bound does not, so the walk
reads past the new end. What it reads there is UNKNOWN. A single-position walk
ends at the first step and is unaffected.

Installed data observation. Across the `en` table the first operand of `0x2B`
is a nested `0x28` token in 13 of 14 tokens, and of `0x2D` in 6 of 9. Row 231
uses `0x2B` for the class name and `0x2D` for a second class name in `en`, and
`0x2B` alone in `fr`. The `de` and `ja` rows use neither.

## Reproduction bounds

Verify the executable identity and PE32 base. Read the vtable `0x01108ECC`
and the table `0x00CB3218` as little-endian dwords. Read the two exception
entries at `0x01108EA4` and the strings they point to. Track each push and
callee cleanup, because the decompiler lists the handler parameters
incorrectly. Keep executable bytes, generated listings and function bodies
outside published artifacts.

| Purpose | Ranges and data |
|---|---|
| Handler registration | `0x00CA0390..0x00CA0898`, thunk bytes at each thunk address above, vtable `0x01108ECC` plus `0xA0` bytes |
| Null handler copy | `0x00CB27CC..0x00CB2813`, `0x00CB2818..0x00CB28D5` |
| Comparison leads | `0x00CB2E86..0x00CB3073`, table `0x00CB3218` plus 24 bytes |
| Sheet handler | `0x00CA0E20..0x00CA0FF3`, `0x00CB3480..0x00CB36EB`, `0x00CA0930..0x00CA0A25` |
| Case handlers | `0x00C9FC20..0x00C9FD3F`, `0x00C9FD50..0x00C9FE0F`, `0x00445220..0x004453A5` |
| Language and exceptions | `0x00C98150..0x00C98154`, `0x01108EA4` plus 24 bytes, `0x01108E70` plus 4 bytes, `0x01108E78` plus 3 bytes |
| Installed sheet | Definition resource `0x0B450269` and the `xtx/_text_ui` definition `0x0B450082` for the per-language fields and the row 231 operands |
