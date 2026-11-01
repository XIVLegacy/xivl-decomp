# LogControl line format and category filter

This page records how a stored chat line becomes a line of a LogControl: which categories a control admits, which text row formats the line, and which strings that row receives. The stored line, the module and the document reader are in [LogControl and pool feed static contract](logcontrol-pool-feed.md). The Map `0x0003` store is in `XIVLegacy/xivl-opcodes:data/s2c_0003_wire_layout.json`. The SendLog records are in [SendLog sink chain](../script/sendlog-sink-chain.md).

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

The property change callback `0x00593F90` switches on the descriptor index stored at `[[arg + 0x70] + 0x14]`. Index 3, `Categories`, reads the property value and runs `0x00593980`, which calls `0x00592160(0)` and then the document filter `0x00591F80`. `0x00592160(0)` clears the document counters and stores `-1` at document `+0x17C`, the index of the last stored line the document has read, and `0x00593980` then stores the result of module slot 4 minus one there, at least `-1`. The filter that follows therefore reads the lines the module still retains again, and the meaning of slot 4 is not traced beyond that use. `0x00592160` stores the module line count at `+0x17C` instead when its argument is 1. Indices 6 and 7 only set bit 1 of the control flag byte at `+0x1F0C`. A write of `LogFormat` or `LogFormat2` reaches no case of this handler. STATIC.

## LF-02: Filter gate

The filter `0x00591F80` adds no line while the control's `Categories` vector at control `+0x1A5C` (begin) and `+0x1A60` (end) is empty: its loop only advances the read index. With a nonempty vector it reads each unread stored line through `0x004C3F10`, searches the 32-bit entries of the vector for the recovered 16-bit category, and calls `0x004571F0` for the line only when an entry equals it. A control with no categories shows no line. STATIC. The pool feed page records the same comparison. The string-to-vector conversion of the `Categories` value is not traced, so the separators and the number base it accepts are UNKNOWN.

## LF-03: The line builder

The function `0x00592940` builds the text of one stored line. Its arguments are the document, the line index, an output string, an output byte and an output 16-bit category. It returns without a line when the index is past the module line count read through document `+0xB0` slot 3 or when `0x004C3F10` fails (`0x005929D1..0x00592A3A`). STATIC.

`0x004C3F10` recovers the category from the leading four hex digits of the stored line, then splits the remainder at the first colon found from the fifth character, so the first part is the sender string and the rest is the text string (`0x004C3F10` calls `0x004555A0` on the digits and `0x006BE5E0` for the colon). The builder then calls a virtual slot at `+8` of the object that `0x004D74E0` returns with the sender as an argument, and the effect of that call on the sender is not traced (`0x00592AE0..0x00592B05`). STATIC for the call, UNKNOWN for the effect.

The builder tests the sender with `0x004451F0`, which returns the string size field minus one (`MOV EAX, [ECX + 8]`, `SUB EAX, 1`). Zero selects the unnamed path and any other value the named path (`0x00592B1D..0x00592B2C`):

| Path | Output byte | Format property | Template |
|---|---|---|---|
| Named | set to 1 (`0x00592B32`) | `LogFormat` at control `+0x19A0` (`0x00592B51`) | `%s:%s` string at `0x00FA8DC8` |
| Unnamed | left 0 | `LogFormat2` at control `+0x19A8` | `{TEXT}` string |

STATIC. Whether a Map `0x0003` line with an empty sender field reaches the unnamed path depends on how the stored line carries the 32 padding bytes of the sender, which is not established (the wire layout file records the padding as carried into the stored line), so which path such a line takes is UNKNOWN.

## LF-04: The row is the property value plus the category

Both paths call `0x005928E0(property, category, out, sender, text)`. It reads the first character of the property string and, when that is a decimal digit, converts the whole string with `_atol`, adds the 16-bit category to it, and calls `0x00592760` with that sum as the text row, the output string, the sender and the text. It returns 1, and the builder then skips its placeholder replacement (`0x005928E0`, `0x00592B7D..0x00592B87`). When the first character is not a digit it returns 0 and the builder falls back to `0x00590830`, which picks the comma separated item of the property string at the category's position, and then replaces the placeholders `{NAME}` and `{TEXT}` in it (named path) or `{TEXT}` (unnamed path). STATIC.

So for a numeric `LogFormat` the text row of a line is `LogFormat + category`, and for a numeric `LogFormat2` it is `LogFormat2 + category`. There is no clamp or table between the category and the row.

`0x00592760` wraps the sender and then the text as string arguments, in that order, into a parameter set (`0x004D3420` and `0x004D50D0` twice), and calls `0x004B58B0` with the row. That function loads the object at `ECX + 0x268` and calls its virtual slot at `+0xC` with the key stored at `ECX + 0x288`, the row, and the parameter set (`0x004B58B0..0x004B58D4`). The caller's `ECX` was not followed, so the sheet and the key are not read here. INFERRED: it is the TextModule lookup of the [ordinary text lookup contract](logcontrol-text-lookup.md) on the default text sheet, which is `xtx/_text_ui` where the installed LogWidget values below name rows. The row text is then expanded by the formatter with the supplied vector `[sender, text]`.

## LF-05: Installed LogWidget values

OBSERVED (private install). The form declares both `LogControl_Main_1` and `LogControl_Main_2` with `LogFormat="2400"`, `LogFormat2="2600"`, `IndentHasName="@2206"`, `IndentNoName="@2207"`, `LinesPerSec="3"`, `LinesPerSecHi="30"`, `IsEnableKeyWait="False"`, `FontSize="14"`, `FontEdge="True"`, `FontEdgeColor="#ff262626"`, `Margin="8,4,4,4"` and `VerticalScrollBarVisibility="AutoHidden"`. The script function `LogWidget.updateLogFormat` (unluac lines 878 to 914) writes `LogFormat` 2400 and `LogFormat2` 2600 to both controls and then `App.InvalidateRender`. Rows 2206 and 2207 of the `en` table are plain numbers.

OBSERVED (private install). The script function `LogWidget.updateLogCategoryDisplayFromUserConfig` (unluac lines 1093 to 1216) builds the string of the literals 28, 29, 30, 31, 34, 35, 36, 37, 38, 39 and 40 joined by commas. For each tab 1 and 2 it uses that string only when the window index `work.index` is 1 and the tab is 1, and the empty string otherwise. It appends a comma and the result of `getConfigLogFilterData(index, tab)` when that result is not the empty string, and calls `setLogCategories` with `LogControl_Main_<tab>` and the string. So the `Categories` value of the first tab of the first window starts with the categories 28 to 31 and 34 to 40, and every other control takes only the filter data. The filter data is read from user configuration and its default is UNKNOWN here. The function `updateDisplayStatus` (unluac lines 1218 to 1253) returns at once for window index 1 and shows a window of another index only when `getConfigLogEnable` is true for one of its two tabs.

OBSERVED (private install). Rows 2400 to 2449 and 2600 to 2649 of `xtx/_text_ui` in `en` each hold, under the colour tokens, uses of the one-based supplied string tokens. The rows from 2600 use only supplied string 2 (the text). Most rows from 2400 use supplied string 1 (the sender), a separator literal that differs by row, and supplied string 2, and rows 2428 and 2429 use supplied string 2 alone. The rows 2414 to 2421 hold a token that has no evaluated handler in the formatter used as a lead. The colour operands are literal in some rows and the `0xE9` global parameter reference of [Rich-text global parameter vector](rich-text-global-parameters.md) in others, so the colour of a category is the colour state of its row. Which category a given row serves follows LF-04 and is not tied here to a channel name.

## LF-06: What stays open

- The colour a category draws in. The row carries it as a colour state, and the value of a `0xE9` operand depends on the unresolved vector of the [Rich-text global parameter vector](rich-text-global-parameters.md).
- The wrap rule, the hanging indent that `IndentHasName` and `IndentNoName` name, the scroll bar and scroll position, the `LinesPerSec` reveal and the retained line count of the document. None is read here.
- The string-to-vector parse of `Categories`, the default filter data, and the category values a server sends.
- Whether an empty sender takes the named path (LF-03).
- The effect of the call through `0x004D74E0` on the sender (LF-03).

## Reproduction bounds

Verify the executable identity and PE32 base. Read the registrations at `0x00F0FB50..0x00F0FBA9`, the thunks at `0x00595235..0x005952B0`, the handler `0x00593F90`, `0x00593980`, `0x00592160`, the filter `0x00591F80`, the builder `0x00592940`, `0x004C3F10`, `0x005928E0`, `0x00590830`, `0x00592760`, `0x004B58B0` and `0x004451F0`. Track each push and callee cleanup, because the decompiler lists the thiscall parameters of the builder incorrectly. Keep executable bytes, generated listings and function bodies outside published artifacts.

| Purpose | Ranges and data |
|---|---|
| Property registration | `0x00F0FB50..0x00F0FBA9`, descriptors `0x0133C3AC`, `0x0133C3D0`, `0x0133C3F4`, names `0x00FA8E18`, `0x00FA8E24`, `0x00FA8E30` |
| Property thunks and handler | `0x00595235..0x005952B0`, `0x00593F90`, `0x00593980`, `0x00592160` |
| Filter | `0x00591F80`, vector at control `+0x1A5C` and `+0x1A60` |
| Builder | `0x00592940` (`0x005929D1..0x00592B87`), `0x004C3F10`, `0x004451F0` |
| Row selection | `0x005928E0`, `0x00590830`, `0x00592760`, `0x004B58B0` |
