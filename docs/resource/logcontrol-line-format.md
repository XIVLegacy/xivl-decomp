# LogControl line format and category filter

This page records how a stored chat line becomes a line of a LogControl: which categories a control admits, which text row formats the line, and which strings that row receives. The stored line, the module and the document reader are in [LogControl and pool feed static contract](logcontrol-pool-feed.md). The Map `0x0003` store is in `XIVLegacy/xivl-opcodes:data/s2c_0003_wire_layout.json`. The text formatter that several steps below call is in [LogControl ordinary text lookup](logcontrol-text-lookup.md). The SendLog records are in [SendLog sink chain](../script/sendlog-sink-chain.md).

This page does not establish the colour a category draws in, the wrap rule, the scroll bar behaviour, the hanging indent, the string-to-vector parse of the `Categories` property, the user configuration default of the filter data, or any rendered line.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | `ghidra-cli 0.2.2` disassembly, cross-reference sets and decompilation as a lead |
| LogWidget script | `client/script/n1635q/yv3n1635q.le.lpb`, 19090 bytes, SHA-256 `d99be24e3e64e9d105b25b722d119ee349c5f9244ddddf4e2e1f048c32355ad7`. The decoded chunk is 19077 bytes, SHA-256 `f40fdae854b956352b045242defcfbedf964944bd2219dc621f9f802600b9a91`. It was decoded with `XIVLegacy/xivl-client-scripts:tools/retail_script.py` and its pinned unluac, so the line numbers below are lines of that unluac output |
| Form | `client/sqwt/widget/LogWidget.form`, decoded with the SQEX Blowfish rule of `XIVLegacy/xivl-tools:docs/formats/sqex.md` |
| Text sheet | `xtx/_text_ui`, read in the installed `en` table through the formatter of `XIVLegacy/xivl-tools` as a lead, and not decompiled |

All addresses are VA. Executable size and SHA-256 were checked first. Every native claim was read from the disassembly. This was static analysis, without client execution.

Evidence labels:

- STATIC: read directly from instructions or data at the cited address.
- OBSERVED (private install): read from a decoded retail file. No row text is recorded here.
- INFERRED: a reading that joins static facts with a convention. The join is stated.
- UNKNOWN: not established.

## LF-01: Property indices and change handler

The static initializer near `0x00F0FB50` registers the LogControl property names with one descriptor object each, and each registration stores the descriptor index after the call: `Categories` (`0x00FA8E18`) descriptor `0x0133C3AC` index 3, `LogFormat` (`0x00FA8E24`) descriptor `0x0133C3D0` index 4 and `LogFormat2` (`0x00FA8E30`) descriptor `0x0133C3F4` index 5 (`0x00F0FB50..0x00F0FBA9`). The property accessor thunks at `0x00595235`, `0x0059525F` and `0x00595289` pass those descriptors and address the property slots at control `+0x1998`, `+0x19A0` and `+0x19A8`. So `+0x19A0` holds `LogFormat` and `+0x19A8` holds `LogFormat2`. STATIC.

The property change callback `0x00593F90` switches on the descriptor index stored at `[[arg + 0x70] + 0x14]`. Index 3, `Categories`, reads the property value and runs `0x00593980`, which calls `0x00592160(0)` and then the document filter `0x00591F80`. `0x00592160(0)` clears the document counters and stores `-1` at document `+0x17C`, the index of the last stored line the document has read, and `0x00593980` then stores the result of module slot 4 minus one there, at least `-1`. Module slot 4 (`0x00C9A8C0`) returns `-1` when module `+0x10` is zero and the dword at module `+0x14` otherwise, and the meaning of `+0x14` is not traced beyond that use. The filter that follows reads the stored lines after that index again. `0x00592160(1)` stores the result of module slot 3 at `+0x17C` instead. Slot 3 (`0x00C9A8B0`, `MOV EAX, [ECX + 0x10]` then `SUB EAX, 1`) returns the highest valid line index, which is the module line count minus one, so only lines stored later are read. The line reader slot 9 (`0x00C9A9F0`) rejects an index at or past `[module + 0x10]`, which is the count that slot 3 reduces by one. The module is the `RaptureLogModule` that constructor `0x004DEE90` installs with table `0x00F91434`. Indices 6 and 7 only set bit 1 of the control flag byte at `+0x1F0C`. A write of `LogFormat` or `LogFormat2` reaches no case of this handler. STATIC.

## LF-02: Filter gate

The filter `0x00591F80` adds no line while the control's `Categories` vector at control `+0x1A5C` (begin) and `+0x1A60` (end) is empty: its loop only advances the read index. With a nonempty vector it reads each unread stored line through `0x004C3F10`, searches the 32-bit entries of the vector for the recovered 16-bit category, and calls `0x004571F0` for the line only when an entry equals it. A control with no categories shows no line. STATIC. The pool feed page records the same comparison. The string-to-vector conversion of the `Categories` value is not traced, so the separators and the number base it accepts are UNKNOWN.

## LF-03: The line builder

The function `0x00592940` builds the text of one stored line. It is a thiscall on the document with five stack arguments (`RET 0x14`): the line index, an output string, an output byte, an output 16-bit category and a second output string that receives a copy of the sender (`0x0059297B..0x00592999`). It returns 0 without a line when the index is greater than module slot 3 read through document `+0xB0`, which rejects every index at or past the module line count, or when `0x004C3F10` fails (`0x005929D1..0x00592A3A`). STATIC.

The stored line is the four digit uppercase hex of the display type, a colon, the sender, a colon and the text (`0x004CE760`, format string `0x00F8D638`, separator string `0x0132F330` initialised to `:` from `0x00F8D634` by `0x00F076E0`). `0x004C3F10(index, category out, sender out, text out)` fetches the line through module slot 9 and parses its first four characters as hex (`0x004555A0`, base 16) into the category. It then searches for the separator from zero based index 5 (`PUSH 5` at `0x004C4024`), so the colon at index 4 is never tested. The sender is the characters from index 5 up to the colon found and the text is everything after it, and a line with no colon at or after index 5 fails. The reader then passes the text substring through slot +8 of the object stored at module `+0x5C`, with the substring, the module `+0x60` parameter set and the text output string (`0x004C40CD..0x004C40DF`), and a zero result fails the reader at `0x004C40F6`. Constructor `0x004DEE90` stores its second argument at `+0x5C` (`0x004DEEE7`) and its caller `0x004DC5D0` passes `RaptureElementContainer + 0x554` (`0x004DC653..0x004DC65D`), the TextModule of the [ordinary text lookup](logcontrol-text-lookup.md), so slot +8 is the formatter `0x00CA1D60`. The text the builder receives is that formatter output and a line whose text it rejects is unreadable. The filter `0x00591F80` stops at such a line without advancing its read index (`0x0059205F..0x005920FC`). STATIC. The rejection conditions of the formatter beyond an empty or plain cell are not traced.

The builder then handles the sender in this order. STATIC.

1. It compares the sender with the string `<debug>` at `0x00FA8DC0` (`0x00445D60`, which returns true on equality) and replaces it with the empty string at `0x00FA8906` (`0x004489C0`) on a match (`0x00592A3C..0x00592A5D`).
2. It copies the sender into the fifth argument (`0x00592A8A..0x00592AA1`). That copy keeps the sender as it stands before step 4.
3. It clears the local sender string with `0x00445530`, which sets its size field to 1 and its first byte to 0 (`0x00592AC1`).
4. It calls slot +8 of the object `0x004D74E0` returns for `[[document + 0x70] + 0x1B08]` with the copy, a parameter set local and the local sender as the output (`0x00592AE0..0x00592B05`). `0x004D74E0` is `LEA EAX, [ECX + 0x554]`, so this is the TextModule formatter `0x00CA1D60` of the [ordinary text lookup](logcontrol-text-lookup.md) again.
5. It tests the local sender with `0x004451F0`, which returns the string size field minus one (`MOV EAX, [ECX + 8]`, `SUB EAX, 1`). Zero selects the unnamed path and any other value the named path (`0x00592B1D..0x00592B2C`).

So the named or unnamed decision, the `{NAME}` text (`0x00592C20`) and the sender argument of `0x005928E0` all come from the formatter output and not from the raw sender. For a plain cell the formatter copies the bytes before the first NUL to the output, and an empty cell returns success with the output untouched (`0x00CB27A7..0x00CB27AF` to `0x00CB2A9D`). A sender whose first byte is NUL, an empty sender and a `<debug>` sender therefore leave the output empty and take the unnamed path, whether or not the NUL padding of the Map `0x0003` sender field is carried into the stored line. A padded short name yields the name without its padding. A sender holding a byte `0x02` is not a plain cell and its output is not traced here. INFERRED for the padded cases, from the plain branch of the formatter.

| Path | Output byte | Format property | Template |
|---|---|---|---|
| Named | set to 1 (`0x00592B32`) | `LogFormat` at control `+0x19A0` (`0x00592B51`) | `%s:%s` string at `0x00FA8DC8` |
| Unnamed | left 0 | `LogFormat2` at control `+0x19A8` | `{TEXT}` string at `0x00FA8DD4` |

STATIC. Every path ends by appending the string `"\n"` to the output string through `0x00448900` and returning 1: `0x00FA8DD0` on the named path and `0x00FA8DDC` on the unnamed path, including the numeric row paths (`0x00592B87` to `0x00592D17` and `0x00592D72` to `0x00592E67`, then `0x00592E6C`). STATIC.

## LF-04: The row is the property value plus the category

Both paths call `0x005928E0(property, category, out, sender, text)`. It reads the first character of the property string and, when that is a decimal digit, converts the whole string with `_atol`, adds the 16-bit category to it, and calls `0x00592760` with that sum as the text row, the output string, the sender and the text. It returns 1, and the builder then skips its placeholder replacement (`0x005928E0`, `0x00592B7D..0x00592B87`). When the first character is not a digit it returns 0 and the builder falls back to `0x00590830`, which picks the comma separated item of the property string at the category's position, and then replaces the placeholders `{NAME}` and `{TEXT}` in it (named path) or `{TEXT}` (unnamed path). STATIC.

So for a numeric `LogFormat` the text row of a line is `LogFormat + category`, and for a numeric `LogFormat2` it is `LogFormat2 + category`. There is no clamp or table between the category and the row.

`0x00592760(row, out, sender, text)` wraps the sender and then the text as string arguments, in that order, into a parameter set (`0x004D3420` and `0x004D50D0` twice), and calls `0x004B58B0` on the object `ECX` holds. `0x00592760` loads that `ECX` from the static `[0x0133C638]`, which it initialises to `[0x0133C63C] + 0x3C`, where `[0x0133C63C]` is the result of `0x00974220`, the global at `0x0135C2AC` (`0x005927A3..0x005927CC`, `0x005928AA`). `0x004B58B0` pushes the address `ECX + 4`, the row, the dword at `ECX + 0x288`, the parameter set and the output string, and calls slot +0xC of the object at `ECX + 0x268` with them (`0x004B58B1..0x004B58D8`). That is the five argument contract of `0x00CA0930` in the [ordinary text lookup contract](logcontrol-text-lookup.md): key K, row R, column selector, parameter set P and output O. So `ECX + 0x288` is the column selector and the key K is at `ECX + 4`. The out string is the output O. STATIC for the argument order. Whether the object at `ECX + 0x268` is the TextModule, the value of K and the value of the column selector are not traced here, so which sheet the log rows come from is UNKNOWN on this chain. The installed LogWidget rows named in LF-05 were read in `xtx/_text_ui` as a lead. The row text is then expanded by the formatter with the supplied vector `[sender, text]`.

## LF-05: Installed LogWidget values

OBSERVED (private install). The form declares both `LogControl_Main_1` and `LogControl_Main_2` with `LogFormat="2400"`, `LogFormat2="2600"`, `IndentHasName="@2206"`, `IndentNoName="@2207"`, `LinesPerSec="3"`, `LinesPerSecHi="30"`, `IsEnableKeyWait="False"`, `FontSize="14"`, `FontEdge="True"`, `FontEdgeColor="#ff262626"`, `Margin="8,4,4,4"` and `VerticalScrollBarVisibility="AutoHidden"`. The script function `LogWidget.updateLogFormat` (unluac lines 878 to 914) writes `LogFormat` 2400 and `LogFormat2` 2600 to both controls and then `App.InvalidateRender`. Rows 2206 and 2207 of the `en` table are plain numbers.

OBSERVED (private install). The script function `LogWidget.updateLogCategoryDisplayFromUserConfig` (unluac lines 1093 to 1216) builds the string of the literals 28, 29, 30, 31, 34, 35, 36, 37, 38, 39 and 40 joined by commas. For each tab 1 and 2 it uses that string only when the window index `work.index` is 1 and the tab is 1, and the empty string otherwise. It appends a comma and the result of `getConfigLogFilterData(index, tab)` when that result is not the empty string, and calls `setLogCategories` with `LogControl_Main_<tab>` and the string. So the `Categories` value of the first tab of the first window starts with the categories 28 to 31 and 34 to 40. Every other control takes the empty string, a comma and the filter datum, so a nonempty datum arrives with a leading comma (unluac lines 1185 to 1197). Whether the unparsed `Categories` conversion accepts the empty first token is open. The filter data is read from user configuration and its default is UNKNOWN here. The function `updateDisplayStatus` (unluac lines 1218 to 1253) returns at once for window index 1 and shows a window of another index only when `getConfigLogEnable` is true for one of its two tabs.

OBSERVED (private install). Rows 2400 to 2449 and 2600 to 2649 of `xtx/_text_ui` in `en` each hold, under the colour tokens, uses of the one-based supplied string tokens. The rows from 2600 use only supplied string 2 (the text). Most rows from 2400 use supplied string 1 (the sender), a separator literal that differs by row, and supplied string 2, and rows 2428 and 2429 use supplied string 2 alone. The rows 2414 to 2421 hold a token that has no evaluated handler in the formatter used as a lead. The colour operands are literal in some rows and the `0xE9` global parameter reference of [Rich-text global parameter vector](rich-text-global-parameters.md) in others, so the colour of a category is the colour state of its row. Which category a given row serves follows LF-04 and is not tied here to a channel name.

## LF-06: What stays open

- The colour a category draws in. The row carries it as a colour state, and the value of a `0xE9` operand depends on the unresolved vector of the [Rich-text global parameter vector](rich-text-global-parameters.md).
- The wrap rule, the hanging indent that `IndentHasName` and `IndentNoName` name, the scroll bar and scroll position, the `LinesPerSec` reveal and the retained line count of the document. None is read here.
- The string-to-vector parse of `Categories`, the default filter data, and the category values a server sends.
- The formatter's rejection conditions and its output for a sender or text holding a byte `0x02` (LF-03).
- The identity of the object at `ECX + 0x268` in `0x004B58B0`, and the values of K and the column selector there (LF-04).
- Whether the `Categories` conversion accepts an empty first token (LF-05).

## Reproduction bounds

Verify the executable identity and PE32 base. Read the registrations at `0x00F0FB50..0x00F0FBA9`, the thunks at `0x00595235..0x005952B0`, the handler `0x00593F90`, `0x00593980`, `0x00592160`, the filter `0x00591F80`, the builder `0x00592940`, `0x004C3F10`, `0x005928E0`, `0x00590830`, `0x00592760`, `0x004B58B0`, `0x004451F0`, `0x00445D60`, `0x004D74E0`, `0x00CA1D60`, `0x00CB2760`, `0x004CE760`, `0x004DEE90`, `0x004DC5D0`, `0x00974220` and the module slots `0x00C9A8B0`, `0x00C9A8C0` and `0x00C9A9F0`. Track each push and callee cleanup, because the decompiler lists the thiscall parameters of the builder incorrectly. Keep executable bytes, generated listings and function bodies outside published artifacts.

| Purpose | Ranges and data |
|---|---|
| Property registration | `0x00F0FB50..0x00F0FBA9`, descriptors `0x0133C3AC`, `0x0133C3D0`, `0x0133C3F4`, names `0x00FA8E18`, `0x00FA8E24`, `0x00FA8E30` |
| Property thunks and handler | `0x00595235..0x005952B0`, `0x00593F90`, `0x00593980`, `0x00592160` |
| Filter | `0x00591F80`, vector at control `+0x1A5C` and `+0x1A60` |
| Builder | `0x00592940` (`0x005929D1..0x00592E6C`), `0x004C3F10`, `0x004451F0`, `0x00445D60`, strings `0x00FA8DC0`, `0x00FA8DD0`, `0x00FA8DDC` |
| Stored line and module | `0x004CE760`, `0x004DEE90`, `0x004DC5D0`, table `0x00F91434`, slots `0x00C9A8B0`, `0x00C9A8C0`, `0x00C9A9F0`, separator `0x0132F330` |
| Sender formatter | `0x004D74E0`, `0x00CA1D60`, `0x00CB2760` (`0x00CB27A7..0x00CB2A9D`) |
| Row selection | `0x005928E0`, `0x00590830`, `0x00592760`, `0x004B58B0`, `0x00974220`, statics `0x0133C638`, `0x0133C63C`, `0x0135C2AC` |
