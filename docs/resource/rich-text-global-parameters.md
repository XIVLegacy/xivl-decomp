# Rich-text global parameter vector

The operand lead `0xE9` of the rich-string decoder reads an integer from a vector that the TextModule owns. This page records where that vector lives, which code seeds it, which code writes the chat log colours into it, the defaults of those colours and which vector elements the colour tokens of the `xtx/_text_ui` title rows select. The operand leads and the colour handlers are in [Rich-text colour tokens](rich-text-colour-tokens.md), CT-02 and CT-03. The formatter and its handler vector are in [Rich-text sheet and case handlers](rich-text-sheet-and-case-handlers.md), RT-01.

This page does not establish the values a player has saved in the config store, whether the config change handler runs once at load, the meaning of the elements outside the colour range, or any colour in a rendered frame.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | `ghidra-cli 0.2.2` disassembly, decompilation as a lead, memory reads of vtables, literals and the string at `0x00FD87C4` |

All addresses are VA. Subtract `0x00400000` for RVA. Executable size and SHA-256 were checked first. Every instruction-level claim was read from the disassembly and every table cell and literal from the executable's data. This was static analysis, without client execution.

Evidence labels:

- STATIC: read directly from instructions or data at the cited address.
- INFERRED: a reading that joins static facts with a convention. The join is stated.
- UNKNOWN: not established.

Installed-data observations give token ids, operand kinds and row ids and carry no row text.

## GP-01: Vector identity

The TextModule class `Component::Text::TextModule` has the primary vtable `0x01108F70`. Its slot 1 (`0x01108F74`) is `FUN_00C0EA60`, which is `LEA EAX,[ECX+0x18]` and `RET`. The constructor `FUN_00CA0390` (`0x00CA0390..0x00CA0898`) stores the decoder sub-object at TextModule `+4` (vtable `0x01108ECC`) and calls the decoder constructor `FUN_00CB33A0` at `0x00CA03CD`. So the deque that `0xE9` and `0xEB` read at decoder `+0x14` (CT-02) is the object at TextModule `+0x18`, and slot 1 returns its address. The deque is a standard block deque with the start offset and size at deque `+0xC` and `+0x10`, and `FUN_008A68A0` is its bounds-checked indexed accessor. A cell is a variant with the value at `+4` and a tag byte at `+8`, and the integer reader `FUN_00778D50` requires tag 0. STATIC.

The parameter holder constructor `FUN_00771B80` (`0x00771B80..0x00771C16`) takes a TextModule pointer and a second argument, stores them at holder `+0` and `+4`, allocates a four byte cell with `FUN_009D1B35`, and calls TextModule slot 1 through `CALL EAX` at `0x00771BD8`. It passes the returned deque address to `FUN_007716A0` and stores the cell at holder `+8` (`0x00771BE9`). The cell therefore holds the TextModule deque address, and every holder method reaches the vector through `[[holder+8]]`. STATIC.

The UI data object at container `+0x510` is filled by `FUN_0057A3C0`. The container constructor `FUN_004DBF40` places a TextModule at container `+0x554` (`0x004DC061..0x004DC068`), and the call sites at `0x004D7ACD`, `0x004DB4D5` and `0x004DCF2C` pass that address as the third argument of `FUN_0057A3C0`. At `0x0057A673` that routine calls `FUN_00771B80` with the TextModule and stores the holder at object `+0xC` (`0x0057A6A1`). STATIC.

## GP-02: Seed values

`FUN_007716A0` (`0x007716A0..0x007718E5`) stores the deque address in its cell, calls `FUN_00789B20` with the deque and `0x37` (`0x007716AA..0x007716AF`), and then writes cells through the integer setter `FUN_004D3390` and the string setter `FUN_004D3420`. STATIC.

| Elements (zero based) | Seed | Locator |
|---|---|---|
| 12 to 43 | integer `0xFFF3F3F3` | loop `0x007716C1..0x0077171A`, counter `0x0C..0x2B` |
| 50 to 53 | integer 0 | `0x00771725..0x007718A0` |
| 54 | the empty string `0x01266B10` | `0x007718D2..0x007718D7` |

`FUN_00789B20` builds a default variant with tag `0xFF` and passes it with the count to `FUN_007896F0`, which is not traced. INFERRED: the pair sizes the deque to 55 elements. The initializer does not write elements 0 to 11 and 44 to 49. Their values come from other writers that this page does not trace. STATIC for the writes, UNKNOWN for the other elements.

## GP-03: Colour writer

The config change handler is slot 1 of the vtable `0x00F9117C` (cell `0x00F91180`), `FUN_004DB800`. The container constructor stores that vtable at container `+0x4A4` (`0x004DBFD9`), so the handler runs with the `this` that is container `+0x4A4`, and `this+0x6C` is container `+0x510`. The handler compares its first argument with `this+0x16F8C` (the config store) and ends otherwise. STATIC.

When its group argument is `-1` or `1` (`0x004DB99E..0x004DB9A6`), the handler runs a loop with the counter `i` from 0 to `0x1F` (`0x004DB9B0..0x004DB9C8`):

1. It calls `FUN_004B4F80(i)` with the config store as the receiver. That function calls `FUN_00443E40(1, i, 0)` and subtracts `0x1000000` from the result (`0x004B4F8E`). `FUN_00443E40` returns the dword at entry `+0x60` of the config row at `(group*0x20 + (i & 0x1F)) * 0xBC` from the table at store `+8`, or its third argument when the table is absent (`0x00443E40..0x00443E65`).
2. It calls `FUN_00575510(i, value)` with the receiver `this+0x6C`. That function loads the holder from receiver `+0xC` and tail calls `FUN_0076A7B0`, which writes the integer `value` to deque element `i + 0xC` (`ADD EAX,0xC` at `0x0076A7C6`, accessor call at `0x0076A7CA`, variant store `FUN_004D3390`). STATIC.

For a config value `v` in `0..0xFFFFFF` the subtraction gives `0xFF000000 | v` modulo 2^32, so the element holds an opaque `0xAARRGGBB` dword with the config value as the low 24 bits. The `0x13` handler reads red from bits 16 to 23 (CT-03), so a config value is `0xRRGGBB`. STATIC for the arithmetic, INFERRED for the channel order of the stored config value because the handler is the only reader traced.

`FUN_00575520` (`0x00575520`, tail call to `FUN_0076A7E0`) writes a byte to element `0x2C` (44) the same way. STATIC.

The dispatch of this handler with group `-1` or `1` at config load, and the saved values a player has set, are not traced. Until the handler runs, the colour elements hold the GP-02 seed. UNKNOWN.

## GP-04: Config defaults

`FUN_004C4750` registers the group 1 colour rows through `FUN_00443CF0` (`0x004C67E0..0x004C6D80`). Each call pushes the default, the maximum `0xFFFFFF`, the minimum 0, the index and the group 1, and the register routine stores the default at entry `+0x60`. The names are the strings at `0x00F8D360..0x00F8D4DB`. STATIC.

| Index | Key | Default |
|---:|---|---|
| 0 | `log_color01` | `0xFFFFFF` |
| 1 | `log_color02` | `0xFFA666` |
| 2 | `log_color03` | `0xFF8CC6` |
| 3 | `log_color04` | `0x66E6FF` |
| 4 to 11 | `log_color05` to `log_color12` | `0xD4FF7F` |
| 12 | `logcolor13` | `0x745BA6` |
| 13 | `logcolor14` | `0xA65B80` |
| 14 | `logcolor15` | `0xF3F3F3` |
| 15 | `logcolor16` | `0x878787` |
| 16 | `logcolor17` | `0xA6904B` |
| 17 | `logcolor18` | `0xA65353` |
| 18 | `logcolor19` | `0xA6904B` |
| 19 | `logcolor20` | `0xA65353` |
| 20 | `logcolor21` | `0xA6904B` |
| 21 | `logcolor22` | `0xA6904B` |
| 22 to 25 | `logcolor23` to `logcolor26` | `0x4295A6` |
| 26 to 31 | `logcolor27` to `logcolor32` | `0x878787` |

The key spelling changes from `log_color12` to `logcolor13`. The strings are in the executable as listed.

## GP-05: The 0xE9 join

`FUN_00CB2D50` lead `0xE9` decodes its operand, subtracts one and reads the deque element at that zero based index, or yields zero outside the deque size (CT-02). The deque holds 55 elements after GP-02 (INFERRED in GP-02). So the operand `N` selects zero based element `N - 1`, and the colour range is `N = 13` to `44`, which are colour indices `N - 13` of GP-04. STATIC for each step, INFERRED for the join of the holder deque and the decoder deque because the pointer in the holder cell comes from slot 1 of the same TextModule and no other store to the cell was found.

The installed `xtx/_text_ui` rows 2227 to 2238 hold one `0x13` token each whose operand is `0xE9` over the literal 13 to 24 in order (CT-04). Those literals select colour indices 0 to 11, which are the keys `log_color01` to `log_color12`. They do not select `logcolor13` to `logcolor24`. The numbering agreement that CT-04 recorded is a coincidence of the twelve element offset, and the keys that carry the same twelve digits are a different set. The literal 13 of row 2227 selects index 0, whose default is `0xFFFFFF`. The token operand after the config handler runs is therefore the dword `0xFFFFFFFF` for an unchanged config, and the `0x13` handler stores red, green and blue 1.0 (CT-03). Before the handler runs it is `0xFFF3F3F3`. The operand is the same in all four installed languages, so the colour does not depend on the text locale. INFERRED, from STATIC steps with the UNKNOWN dispatch of GP-03.

The other 24 `0xE9` tokens of the rows 2401 to 2424 carry the same literals 13 to 24, so they select the same twelve colours.

## Reproduction bounds

Verify the executable identity and PE32 base. Read the vtables `0x01108F70`, `0x01108ECC` and `0x00F9117C` as little-endian dwords, the string at `0x00FD87C4` and the config names at `0x00F8D360`. Track each push and callee cleanup, because the decompiler lists several of these thiscall parameters incorrectly. Keep executable bytes, generated listings and function bodies outside published artifacts.

| Purpose | Ranges and data |
|---|---|
| Vector and holder | `0x00C0EA60`, `0x00CA0390..0x00CA03DF`, `0x00771B80..0x00771C16`, `0x007716A0..0x007718E5`, `0x0057A3C0` (`0x0057A64B..0x0057A6A1`) |
| Container placement | `0x004DBF40` (`0x004DBFD9`, `0x004DC061..0x004DC068`), `0x004D7ACD`, `0x004DB4D5`, `0x004DCF2C` |
| Colour writer | `0x004DB800` (`0x004DB99E..0x004DB9C8`), `0x004B4F80`, `0x00443E40`, `0x00575510`, `0x00575520`, `0x0076A7B0`, `0x0076A7E0`, `0x004D3390` |
| Config rows | `0x004C4750` (`0x004C67E0..0x004C6D80`), `0x00443CF0`, `0x00F8D360..0x00F8D4DB` |
