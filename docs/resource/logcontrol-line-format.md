# LogControl line format and category filter

This page records how a stored chat line becomes a line of a LogControl: which categories a control admits, which text row formats the line, and which strings that row receives. The stored line, the module and the document reader are in [LogControl and pool feed static contract](logcontrol-pool-feed.md). The Map `0x0003` store is in `XIVLegacy/xivl-opcodes:data/s2c_0003_wire_layout.json`. The text formatter that several steps below call is in [LogControl ordinary text lookup](logcontrol-text-lookup.md). The SendLog records are in [SendLog sink chain](../script/sendlog-sink-chain.md).

[Copied controls](#copied-controls) records how the `0x10`, `0x19` and `0x1A` tokens that the formatter leaves in a line are laid out and drawn.

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
| Body decoder | Python 3.11.15 and Capstone 5.0.7 x86-32, reading the raw PE bytes through the section table |

All addresses are VA. Executable size and SHA-256 were checked first. Every native claim was read from the disassembly. RTTI names were read from the type descriptor at `+0x0C` of the complete-object locator at `[vtable - 4]`. This was static analysis, without client execution.

[Copied controls](#copied-controls) rests on these complete bodies and data, each checked against its SHA-256 before it was decoded.

| Complete body, half-open | Bytes | SHA-256 |
| --- | ---: | --- |
| `[0x00592F30,0x005933E0)` document line insert | 1200 | `d82eea3e55c517317d5fb254a5baf504a39e5d2a78c9cca18cf8306f25e33f26` |
| `[0x00595A20,0x0059615F)` LogControl constructor | 1855 | `6524dcf09df1576737bb4753b939792931491c8cdc786691b6a64f1c94ac2ac0` |
| `[0x0098DAA0,0x0098DBB2)` WrapTextDocument constructor | 274 | `d149ed8c03ef353ba4db8a71751be6807f5bfd6d22813502bbca7c8c25d766fd` |
| `[0x0098D320,0x0098D694)` WrapTextDocument slot +0x50 | 884 | `02c2c8af3de5e47f628403d680906256a9925bb635a478c85335a42a642275fc` |
| `[0x0098C3A0,0x0098C434)` WrapTextElement constructor | 148 | `9d3110a172524a33650064547356625fae3b69884d12c5aec5eb19b4c4808ebc` |
| `[0x00599170,0x0059926F)` FormattedText2 constructor | 255 | `a2ec33b7f59f1004f1f1e55a759a8b3dd4f57b9b417dabd156c0c5c84e5ac6d0` |
| `[0x00988710,0x00988726)` line record getter | 22 | `b20f3469b8d5c637f21d0a4695b1887bb00a5564c2935ef83a9cb8d2893fde47` |
| `[0x00988480,0x0098870C)` text layout | 652 | `03a09102790d2b467eaccf349165e6bba2e3977ef39ef99d6c378880a927e086` |
| `[0x00987FC0,0x0098801E)` font interface dispatch | 94 | `2178616f9185f5a505576ebd8a6e1734881f52072c3d1560b05cc8adc9ee1602` |
| `[0x0055B3E0,0x0055BB4B)` RaptureFontInterface slot 1 | 1899 | `bcdfece1a365275e7fc5ac5289eaa5a3a5f2a6d16e08c112b409f3f34fa2b8d2` |
| `[0x00557810,0x005578E4)` line state builder | 212 | `11ffbc925e0880269c223701803a25db5bf04ca61ed9bcefe3331dcfc5278351` |
| `[0x00CBFE30,0x00CBFE38)` BitmapFont slot +0x30 thunk | 8 | `7d97433cc3322d62d29d395d7134b0999f4cedf7cb13e6c2d86d3ba82504f823` |
| `[0x00CC2190,0x00CC231F)` line fit | 399 | `a902e899a4afee9652b9f081c3a6e1b3f66a14e9b03bececa965efcd83656fe3` |
| `[0x00CC0A40,0x00CC0C3F)` wrap callback | 511 | `3b8d283238a2d90651b94d98d9d522c315b45b49e956a4703c391c3ee34f00ea` |
| `[0x00CC1C50,0x00CC1FD5)` font code processor | 901 | `18fa0f75188fffca2c0b6183d6a9870f1b1b4181a3ed5454b6d085c022027ab6` |
| `[0x00CC14B0,0x00CC177F)` FontCodeModule constructor | 719 | `308e8768d1801cfaa741c9129b21ecfb915729ae3d57ca4583ddd8a09ec95c17` |
| `[0x00CBFD10,0x00CBFDB5)` FontCodeStateDecoder constructor | 165 | `c163b6296e921fd1779b618cd44ebcec1f290c0f95ba5ee579bc473bffad23db` |
| `[0x00B241C0,0x00B241C5)` code `0x10` thunk | 5 | `2583df412e7df54ae54a5b27baf8262583a3e4de9ed4ddfd2fef4f3209c53670` |
| `[0x00B241B0,0x00B241B5)` code `0x1A` thunk | 5 | `226cd978c48a410430ca76a02a2c93be5bc97dccfd5b05358faf07fb66d956cf` |
| `[0x00CC0C40,0x00CC0D08)` newline routine | 200 | `a4f65f2580add8b8082f4dbfd07da347fb7b7cb9d1926612b9465ed4592fa911` |
| `[0x00CBFC90,0x00CBFCC6)` code `0x1A` handler | 54 | `299060d777f30f099a6c8aeb3b7907fe01d764a8d69b47d3ba448ef771a34ebe` |
| `[0x00CBFDC0,0x00CBFDC5)` code `0x19` thunk | 5 | `99b17a6d8595935c2979a2eb79d7755c95188af5ea936d771caad4bd3b04d5b8` |
| `[0x00CBFC50,0x00CBFC86)` code `0x19` handler | 54 | `fe8cfb474dbab9e64c2b5b43761d27873b26666e8ae954fe8538ed4b055c2294` |
| `[0x00B24220,0x00B24225)` code `0x18` thunk | 5 | `4ffb2fbf34f9b92062ee1464df1f7bc3d70443c5856122d08dac6603ac71fb04` |
| `[0x00CBFBD0,0x00CBFC0E)` code `0x18` handler | 62 | `19ecfca8650aabeadb224cc56d77af0d525f57a90b0e929ef1f3d6e20f0d59dd` |
| `[0x00B241F0,0x00B241F5)` code `0x1B` thunk | 5 | `21edd959be2e1c8cbcc2106347aaef2f0f8ad414baf0da83e5ec02b31bc7a275` |
| `[0x00CBFCD0,0x00CBFD06)` code `0x1B` handler | 54 | `07747d264d72e0b6d7846ec530ce1f1f133a5809425af200543947a39401e485` |
| `[0x00CB2D50,0x00CB3250)` integer decoder, with its two jump tables from `0x00CB3218` | 1280 | `0797bedb028d975dd049f2511562f81205d6f085ad3459706eba35d2f1407faf` |
| `[0x00CC0670,0x00CC0A0C)` tag emitter | 924 | `de72107df1e21db6b9ae110c0858b423f840c180c856f9c1ef1b356e1d3813db` |
| `[0x00CAC330,0x00CAC6FE)` BitmapFont draw slot +0x0C | 974 | `082d3b2b49b69938c58ee589c55d83317c44b51a69b9a881da8aeb3b42eb6b32` |
| `[0x00CAC700,0x00CACE61)` BitmapFont draw slot +0x14 | 1889 | `0225cd9e2aed404d03864807ff1fbdccc2ae6ecfcd54f76572e4de8697c84423` |
| `[0x00CAD710,0x00CADB49)` OsFont draw slot +0x0C | 1081 | `97dbf8b71d67ce2f9f9754e6dfb88897d9927c5f57526427107c2bd6f7909fb4` |
| `[0x00CADB50,0x00CAE311)` OsFont draw slot +0x14 | 1985 | `3785c83209bfc08ecf437c68b40428c55ab1dc303882e4f5ded024ffc07add6d` |
| `[0x00CA59E0,0x00CA5A35)` glyph quad helper | 85 | `d4c8ca7c6ef9a92bfd64502ff88ed923ddab2ce6cdeb30f582c1d78528817ab1` |
| `[0x00CA54D0,0x00CA5591)` glyph quad helper, first branch | 193 | `bbe0d1a6485a211302d83e1ba8288638da9887cf1e9c4a2f611ae5fa955b2958` |
| `[0x00CB9BB0,0x00CB9EB7)` quad builder | 775 | `10da88e6dabdb9982c85f47d87e21c13904e515049c0148470228372cbe73e10` |

| Data, half-open | Bytes | SHA-256 |
| --- | ---: | --- |
| `[0x00FA2B60,0x00FA2B68)` `RaptureFontInterface` vtable slots 0 and 1 | 8 | `70eaf59371f2e389163c64e5ccbefbdd36f6575b03a89e8ea028afe41e2f2c58` |
| `[0x01077CBC,0x01077CC0)` `WrapTextDocument` vtable slot +0x50 | 4 | `aca509a9bdb97c176e7e72cc146926dd379ca2e6dd872ed0b76d0866d0a9b9f1` |
| `[0x0110C1C4,0x0110C1C8)` `BitmapFont` primary vtable slot +0x30 | 4 | `2c79a169f2b14a905553e362fbf961fc413341aa76928aad801bcf496c5614f6` |
| `[0x0110BEFC,0x0110BF3C)` `FontCodeStateDecoder` vtable slots 0 to 15 | 64 | `4ed53029125bdd34e9a2c611d6a1b929132ba11f81f233993b646775152cda03` |
| `[0x0110C014,0x0110C054)` `FontCodeModule` vtable slots 0 to 15 | 64 | `f4162ba074ebcbe4f1016ac151b38db57d39c3d677dec121f196e5bc3d29b9e0` |
| `[0x0110C0BC,0x0110C0FC)` `BitmapFont` vtable at `+0x10`, slots 0 to 15 | 64 | `46907fe1469f164848f288629767f9c9f3f3dc9c6581ab763cc24bff53f42959` |
| `[0x0110C35C,0x0110C39C)` `OsFont` vtable at `+0x10`, slots 0 to 15 | 64 | `2f8a2edad92298f1af54c517da9221c7f06c2bd8f14a9d956ae9aab44a85ab7b` |
| `[0x0110D95C,0x0110D99C)` `Font` vtable at `+0x10`, slots 0 to 15 | 64 | `078fe4cfb8804a0b31af09b17fc6ac6b5e6a5b170b6815bbe9b6da3b43fb0bff` |
| Slot 16 of the `FontCodeStateDecoder`, `FontCodeModule`, `BitmapFont`, `OsFont` and `Font` vtables, `[0x0110BF3C,0x0110BF40)`, `[0x0110C054,0x0110C058)`, `[0x0110C0FC,0x0110C100)`, `[0x0110C39C,0x0110C3A0)`, `[0x0110D99C,0x0110D9A0)` | 4 each | `b0c97c36d42b8f9fe770838915313fa55e48a83afe0f134ca49b544b7fd13d24` each |
| `[0x00FBF9D0,0x00FBF9D8)` code `0x18` scale double | 8 | `595700f5c0ba132c9ede02f9ad9a19cffe697fd8fd52890abb10fd78f47d9aa1` |
| `[0x0110C168,0x0110C180)` `BitmapFont` draw vtable slots 0 to 5 | 24 | `0a824b694ed0119a0c85b4e939f0fe9474513708a0d7001bca4f649c99fa0e5d` |
| `[0x0110C408,0x0110C420)` `OsFont` draw vtable slots 0 to 5 | 24 | `e559f647e599880e1189b925216e723aad5e3ed5687be076cc8e11b4fde5fe79` |
| `[0x0110BEB4,0x0110BEB8)` BitmapFont slant float | 4 | `ba928cb768da7604c05d4529325bee0501d04e04d7fc900464d889a65d99433a` |
| `[0x0110C314,0x0110C318)` OsFont slant float | 4 | `ba928cb768da7604c05d4529325bee0501d04e04d7fc900464d889a65d99433a` |
| `[0x0110DBB4,0x0110DBB9)` string `</i>` | 5 | `a9bc1a46fe67a9ff2c43fd5b8dba20dd7632c735462d32d03d1f91837b9b3401` |
| `[0x0110DBBC,0x0110DBC1)` string `</b>` | 5 | `af0c8959970c4ef1dce540645f41176003a6e2194866cd7db47ba828d6b17b23` |
| `[0x0110DC18,0x0110DC1C)` string `<b>` | 4 | `01e12f1e5149225494f8e63ab98b5c091a4b36d9afb9808b5d13a984745a6c01` |
| `[0x0110DC1C,0x0110DC20)` string `<i>` | 4 | `20f2a463ec6125eaf2acf45be25180399e7ceb5bbae40855bf73a1f01640962e` |
| `[0x0110DC20,0x0110DC25)` string `<br>` | 5 | `9b0478c090177b5e34fb064a647165fe7c7d4d6ed149aef7a0930799812faac5` |

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

## Copied controls

The formatter copies a token whose TextModule vector entry is null whole to its output, and codes `0x10`, `0x19` and `0x1A` have null entries ([RT-01](rich-text-sheet-and-case-handlers.md#rt-01-handler-vector)). The text LF-03 receives therefore still holds `02 10 01 03`, `02 19 02 <v> 03` and `02 1A 02 <v> 03` byte for byte, and the builder appends `"\n"` after them. STATIC through RT-01. The font code processor interprets the three codes when the line is laid out.

### Entry and rows

`0x00592F30` builds one stored line with the builder `0x00592940` (`0x00592FC5`), wraps the result in a `.?AVFormattedText2@Media@Sqwt@@` (constructor `0x00599170`, vtable `0x00FA94F4` stored at `0x005991A0`, call at `0x005930E2`) and inserts it with one call through slot +0x50 of the object at document `+0x108` (`0x005930FC..0x00593117`). It then gives each row from the old row count at `+0x168` to the new count at `+0x160` its own `0x98` byte attribute copy (`0x00593221..0x00593386`). STATIC.

The LogControl constructor `0x00595A20` calls `0x0098DAA0` at `0x00595ABA` and stores the result at control `+0x1B00` (`0x00595AD1`). `0x0098DAA0` installs vtable `0x01077C6C`, `.?AVWrapTextDocument@DocumentModel@Sqwt@@`, at `0x0098DAF2`, and its slot +0x50 is `0x0098D320`. `0x0098D320` lays the text out at the width at its `+0x7C` (`0x0098D514..0x0098D531`), reads the laid-out line records, 0x1C bytes each, through `0x00988710` (`0x0098D569`), and for each record builds a `.?AVWrapTextElement@DocumentModel@Sqwt@@` with `0x0098C3A0` (vtable `0x01077BAC` stored at `0x0098C3CE`) and inserts it (`0x0098D5C4..0x0098D5DE`, back edge `0x0098D61D`). STATIC. That the object at document `+0x108` is this WrapTextDocument is INFERRED from the matching slot use. So one stored line is one entry, and each laid-out line of it is one row of that entry.

### Line fitting

`0x00988710` runs the layout `0x00988480` when the byte at `+0xC0` is zero (`0x00988713..0x0098871C`), which calls `0x00987FC0` (`0x009884D8`). That routine calls slot +4 of the font interface it holds (`0x00987FCE..0x00988007`). Slot +4 of `.?AVRaptureFontInterface@SqwtInterface@Main@Application@@` vtable `0x00FA2B60` is `0x0055B3E0`. STATIC for the cells. That the held interface is this class is INFERRED.

`0x0055B3E0` builds one line state record P with `0x00557810` at `0x0055B81E`, before its line loop (back edge `0x0055BAB0` to `0x0055B842`), so every line of the text reuses P. The call passes 0, the result of font slot +8 (`0x0055B7B7..0x0055B7C3`) and 0 as the first three stack arguments (`0x0055B81B..0x0055B81D`), which `0x00557810` stores at P `+0x30`, `+0x34` and `+0x38` (`0x00557861`, `0x0055788C`, `0x0055787A`). P `+0` is the flag word (`0x00557845`). Each pass sets P `+0x38` to 0 and P `+0x30` to the line width (`0x0055B8D3..0x0055B8DE`) and calls font slot +0x30 once (`0x0055B8F2`), and when P `+0x38` comes back 0 it retries (`0x0055B8F4..0x0055B930`) after copying P `+0x4C` into P `+0x38` (`0x0055B900..0x0055B907`) and setting P `+0x30` to `0x7FFFFFFF` (`0x0055B916`). It builds the line attribute from P after that call (`0x0055B960..0x0055B967`). STATIC.

BitmapFont primary slot +0x30, cell `0x0110C1C4`, holds `0x00CBFE30`, which adds `0x10` to `ECX` and jumps to `0x00CC2190`. `0x00CC2190` copies P `+0x38` to processor `+0x64` with 0 replaced by `0x7FFFFFFF`, P `+0x30` to the width cap `+0x9C`, P `+0x34` to the height cap `+0xA0` and P `+0` to the flag word `+0x28` (`0x00CC21C6..0x00CC21EB`). It sets the advance `+0x68` to the font slot +8 result plus P `+0x2C` ([BR-03](bitmap-font-runtime-bridge-consumers.md#br-03-composite-getter-and-literal-newline-increment)), clears x at `+0x70` and y at `+0x74` (`0x00CC2234..0x00CC2237`), sets `+0x5C` to 1 (`0x00CC21AD`) and runs the processor `0x00CC1C50` with the wrap callback `0x00CC0A40` (`0x00CC225C..0x00CC2276`). A nonnegative result, -3 or -12 returns the consumed byte count at `+0x60` (`0x00CC227D..0x00CC228B`), and the flag word goes back to P `+0` (`0x00CC2292..0x00CC2295`). The same writeback copies `+0xB8` to P `+0x4C`, `+0xB4` to P `+0x38` and `+0xA8` to P `+0x34` (`0x00CC22C8..0x00CC22E9`). Before the processor runs, the fit sets `+0xA8` to the advance (`0x00CC2249`) and `+0xB8` to 1 (`0x00CC2268`). STATIC.

### Code 0x10

The processor calls a token handler from its own vector ([CT-03](rich-text-colour-tokens.md#ct-03-processor-handlers)). The FontCodeModule constructor `0x00CC14B0` stores thunk `0x00B241C0` for code `0x10` (`0x00CC164A`), and the thunk jumps through vtable +0x10. Cell +0x10 holds `0x00CC0C40` in `.?AVFontCodeModule@Text@Component@@` `0x0110C014`, `.?AVBitmapFont@Font@Component@@` `0x0110C0BC`, `.?AVOsFont@Font@Component@@` `0x0110C35C` and `.?AVFont@0Component@@` `0x0110D95C`. The literal CRLF path (`0x00CC1E09..0x00CC1E1E`) and the literal LF or CR path (`0x00CC1E9E..0x00CC1EB3`) call the same slot. STATIC.

`0x00CC0C40` reads none of its four stack arguments (`RET 0x10` at `0x00CC0CA0` and `0x00CC0D05`). It adds the advance `+0x68` to y, adds 1 to `+0xB4` and sets x to 0 (`0x00CC0C66..0x00CC0C7E`). When the height cap `+0xA0` is nonzero and y is at or past it, it returns -3 (`0x00CC0C85..0x00CC0C8B`). Otherwise it stores y plus the advance at `+0xA8` (`0x00CC0CA3..0x00CC0CA9`), appends `<br>` (`0x0110DC20`) to the string at `+0x200` when `+0x5C` is 0 and that string is set (`0x00CC0CAF..0x00CC0CDD`), and returns 0. The fit sets `+0x5C` to 1, so the layout pass appends no tag. STATIC.

When a handler returns a value from -9 to -1, the processor adds the token's bytes, the length width plus the length plus 3, to `+0x60` and returns that value (`0x00CC1F07..0x00CC1F14`). `02 10 01 03` counts 4 bytes. A literal LF or CR adds 1 and a CRLF adds 2 before the same sign test (`0x00CC1EB5`, `0x00CC1E20`). So the break bytes belong to the line they end. STATIC.

The token path differs from a literal LF in two ways. It stores 1 at `+0x18C` (`0x00CC1D6F`), as the text run path also does (`0x00CC1E53`). After a nonnegative handler result it stops once `+0xB4` reaches the character cap `+0x64` (`0x00CC1DDD..0x00CC1DF2`). `+0xB4` is a character count, because the wrap callback adds run lengths to it (`0x00CC0B47..0x00CC0B52`, `0x00CC0BF2..0x00CC0BF9`) and the text path stops on the same cap (`0x00CC1E62..0x00CC1E72`). STATIC. On the LogControl path P `+0x38` is 0 at the first fit of each pass, so that fit's cap is `0x7FFFFFFF`. The retry's cap is the P `+0x4C` value the first fit wrote back from `+0xB8`, which the wrap callback passes by address to font slot +0x24 (`0x00CC0AEE..0x00CC0AF4`). That value is UNKNOWN.

INFERRED, joining the two font slot +8 calls to one font object. The first fit's height cap is the slot +8 result. Each later fit's cap is the `+0xA8` that the previous fit wrote back to P `+0x34` (`0x00CC22E3..0x00CC22E9`), because the line loop only reads P `+0x34` (`0x0055BA66`). The fit sets `+0xA8` to the advance and the wrap callback sets it to y plus the advance (`0x00CC0AB3`), so after a one-row fit it is the advance. The advance is the slot +8 result plus P `+0x2C`, so after one break y reaches the cap whenever P `+0x2C` is not negative. The newline routine then returns -3 and the fit ends at the break. The wrap callback returns -3 under the same test on entry (`0x00CC0A84..0x00CC0A93`), and on overflow it adds the advance to y and sets x to 0 before it recurses (`0x00CC0B88..0x00CC0BB7`). So `02 10 01 03` ends the current row exactly as a literal LF does, the next row of the same entry starts at x 0 one advance lower, and the token makes no new entry, category or sender. A `0x10` directly before the trailing `"\n"` of LF-03 leaves a last row that holds no glyph.

### Code 0x1A

The FontCodeStateDecoder constructor `0x00CBFD10` stores thunk `0x00B241B0` for code `0x1A` (`0x00CBFD87`), and the FontCodeModule constructor `0x00CC14B0` stores nothing at that vector offset and inherits it from the base constructor `0x00CBFD10`, which it calls at `0x00CC14D9` before it installs vtable `0x0110C014` (`0x00CC14E9`). The thunk jumps through vtable +0x3C. Cell +0x3C holds `0x00CBFC90` in `.?AVFontCodeStateDecoder@Text@Component@@` `0x0110BEFC` and in the four vtables named for code `0x10`. STATIC.

`0x00CBFC90` decodes one integer with `0x00CB2D50` mode 1 (`0x00CBFC95..0x00CBFCA6`). A zero decoded width leaves the flags unchanged (`0x00CBFCAB..0x00CBFCAD`). Otherwise a nonzero value ORs 4 into the flag word `+0x28` (`0x00CBFCB6`) and a zero value ANDs it with `~4` (`0x00CBFCBE`). The handler returns the decoded width and ends with `RET 0x10`. It keeps no saved copy of the flag, unlike the colour handlers of CT-03. STATIC. A literal operand byte decodes to that byte minus 1 ([CT-02](rich-text-colour-tokens.md#ct-02-operand-leads)), so `02 1A 02 02 03` sets italic and `02 1A 02 01 03` clears it, and a second on token is not nested.

An `0xE8` operand reads the processor's local vector, which is zeroed on entry (`0x00CC1C87..0x00CC1C93`) and passed to the handler at `0x00CC1DC2`, so it decodes to 0 and clears the flag. INFERRED, because no handler was seen to fill that vector.

The flag word passes from P `+0` into the processor and back to P after every fit, and P persists across the line loop of `0x0055B3E0`. So italic set on one row stays set across later wraps and breaks of the same entry until an off token, and a break does not clear it. STATIC for each copy.

The tag emitter `0x00CC0670` compares the flag word with its previous copy at `+0x1EC` (`0x00CC06AE..0x00CC06BF`). It writes `</i>` (`0x0110DBB4`) when bit 4 clears and `</b>` (`0x0110DBBC`) when bit 1 clears (`0x00CC06C4`, `0x00CC0704`), and `<b>` (`0x0110DC18`) and `<i>` (`0x0110DC1C`) when bit 1 or bit 4 sets (`0x00CC0943`, `0x00CC0982`). STATIC. Bit 4 is italic and bit 1 is bold, INFERRED from those tag names.

### Code 0x19 and the neighbouring state codes

The FontCodeStateDecoder constructor `0x00CBFD10` stores four thunks into the vector at `+8`. Each store follows a guard that the vector holds more entries than the code (`CMP EAX, code` then `JA`). Each thunk is `MOV EAX, [ECX]` then `JMP [EAX + offset]`, and the cell at that offset holds the same handler in `.?AVFontCodeStateDecoder@Text@Component@@` `0x0110BEFC` and in the four vtables named for code `0x10`. STATIC.

| Code | Guard | Store | Vector offset | Thunk | Jump offset | Handler |
| --- | --- | --- | --- | --- | --- | --- |
| `0x18` | `0x00CBFD34` | `0x00CBFD41` | `+0x60` | `0x00B24220` | `+0x34` | `0x00CBFBD0` |
| `0x19` | `0x00CBFD57` | `0x00CBFD64` | `+0x64` | `0x00CBFDC0` | `+0x38` | `0x00CBFC50` |
| `0x1A` | `0x00CBFD7A` | `0x00CBFD87` | `+0x68` | `0x00B241B0` | `+0x3C` | `0x00CBFC90` |
| `0x1B` | `0x00CBFD9D` | `0x00CBFDAA` | `+0x6C` | `0x00B241F0` | `+0x40` | `0x00CBFCD0` |

For code `0x19` the cells are `0x0110BF34`, `0x0110C04C`, `0x0110C0F4`, `0x0110C394` and `0x0110D994`. Between its base call at `0x00CC14D9` and its return at `0x00CC177C` the FontCodeModule constructor stores only the nine thunks of [CT-03](rich-text-colour-tokens.md#ct-03-processor-handlers), at vector offsets `+0x40` to `+0x5C` and `+0x78`, so it inherits all four entries. The base constructor also clears the flag word `+0x28` (`0x00CBFD1E`). STATIC.

`0x00CBFC50` has the shape of the `0x1A` handler with mask 1. It decodes one integer with `0x00CB2D50` mode 1 (`0x00CBFC66`). A zero decoded width leaves the flags unchanged (`0x00CBFC6B..0x00CBFC6D`). Otherwise a nonzero value ORs 1 into the flag word `+0x28` (`0x00CBFC76`) and a zero value ANDs it with `~1` (`0x00CBFC7E`). It ends with `RET 0x10`. STATIC. By the operand rule of code `0x1A`, `02 19 02 02 03` sets bit 1 and `02 19 02 01 03` clears it. The tag emitter writes `<b>` and `</b>` for bit 1, so code `0x19` is the bold control. INFERRED from those tag names. The TextModule vector has no `0x19` entry, so the token reaches the processor unchanged ([RT-01](rich-text-sheet-and-case-handlers.md#rt-01-handler-vector)).

`0x00CBFCD0` has the same shape with mask 2: it ORs 2 at `0x00CBFCF6` and ANDs with `~2` at `0x00CBFCFE`. STATIC. The tag emitter `[0x00CC0670,0x00CC0A0C)` tests the flag word only for bits 1 and 4, so it writes no tag for bit 2. The pen advance of [Italic drawing](#italic-drawing) adds 1 when flag 2 or flag 1 is set, so codes `0x19` and `0x1B` both reach that branch. STATIC through that read.

`0x00CBFBD0` decodes the same way (`0x00CBFBE6`). On a nonzero width it converts the value to a float, multiplies it by the double at `0x00FBF9D0`, and stores the float result at processor `+0x2C` (`0x00CBFBEF..0x00CBFC05`). The double is `0x3F847AE140000000`, the float 0.01 widened to double. STATIC. Code `0x18` also has a TextModule entry (RT-01), so the formatter does not copy it whole. The consumer of processor `+0x2C` is not traced here.

### Italic drawing

Four draw slots test flag 4: BitmapFont draw vtable `0x0110C168` slots +0x0C (`0x00CAC330`) and +0x14 (`0x00CAC700`), and OsFont draw vtable `0x0110C408` slots +0x0C (`0x00CAD710`) and +0x14 (`0x00CADB50`). Each tests the flag at `0x00CAC37A`, `0x00CAC74E`, `0x00CAD740` or `0x00CADB8F`, then uses the float at `0x0110BEB4` (BitmapFont) or `0x0110C314` (OsFont) as its slant, and 0.0 otherwise. Both floats are `0x3DA3D70A`, which is 0.08. The only code reads of either float are at `0x00CAC39C`, `0x00CAC75C`, `0x00CAD77C` and `0x00CADB9D`. STATIC.

In `0x00CAC330` the glyph height h is a glyph record byte times the vertical scale, stored as the height of the glyph rectangle (`0x00CAC57D..0x00CAC58B`). The draw multiplies h by the slant (`0x00CAC5AD..0x00CAC5B5`) and passes the product s to `0x00CA59E0` (`0x00CAC5F8`). With the dword at `0x013764A0` zero, that helper calls `0x00CA54D0` (`0x00CA5A1C..0x00CA5A1E`), which passes s on to `0x00CB9BB0` (`0x00CA555D..0x00CA5584`). `0x00CB9BB0` reads s at `[esp + 0xB4]` and places the two vertices at y at x + s and x + w + s and the two vertices at y + h at x - s and x + w - s (`0x00CB9C08..0x00CB9C68`). STATIC.

So an italic glyph is the regular glyph image drawn as a parallelogram, with no separate italic font. With y growing downward, the top edge moves by +0.08 h and the bottom edge by -0.08 h, a shear of 0.16 per unit of height about the middle of the glyph. INFERRED for the screen orientation. The OsFont slot +0x0C reads its slant at `0x00CADA3A` and calls the same helper at `0x00CADA8B`. STATIC for those reads, and its height factor is not traced.

The italic advance does not change in the draw. In `0x00CAC330` the slant local is written at `0x00CAC394` and `0x00CAC3A4` and read only at `0x00CAC581`, and the pen advance (`0x00CAC5FD..0x00CAC667`, returned at `0x00CAC6F3`) has no slant term. That advance adds 1 only when the dword at `+0x18` of the FontData at `+0x228` is at most 3 and flag 2 or flag 1 is set (`0x00CAC630..0x00CAC64A`), which is the bold case. STATIC. The wrap callback passes the flag word to the font measure slots +0x1C and +0x24 (`0x00CC0AC4..0x00CC0AC9`, `0x00CC0B06..0x00CC0B0E`), which were not traced, so whether italic changes the measured width is UNKNOWN.

## LF-06: What stays open

- The colour a category draws in. The row carries it as a colour state, and the value of a `0xE9` operand depends on the unresolved vector of the [Rich-text global parameter vector](rich-text-global-parameters.md).
- The wrap rule, meaning where a width overflow splits a run inside the font slot +0x24 fit that the wrap callback `0x00CC0A40` calls, the hanging indent that `IndentHasName` and `IndentNoName` name, the scroll bar and scroll position, the `LinesPerSec` reveal and the retained line count of the document. None is read here.
- The line spacing P `+0x2C` on the LogControl path. The one row per fit reading of [Copied controls](#copied-controls) assumes it is not negative.
- Which flag state a row's draw starts from. Each row attribute holds the state captured after its own fit.
- Whether an entry that ends with italic set starts the next entry in italic. `0x00592F30` seeds a new entry from the last row's attribute only when its slot +0x24 (`0x00591610` in `.?AVLogDocument@CustomControl@SqwtInterface@Main@Application@@` vtable `0x00FA951C`, which returns `+0x8C`) and the row count are both nonzero, and otherwise from the default at `+0x138` (`0x0059307A..0x005930C6`). The value at `+0x8C` is not traced.
- Whether italic changes the measured width, through the untraced measure slots +0x1C and +0x24. How the draw slots +0x14 apply the slant after loading it. The shear on the second branch `0x00CA55A0` of `0x00CA59E0`, taken when the dword at `0x013764A0` is nonzero.
- The character cap of the retry fit, which is the P `+0x4C` value that font slot +0x24 leaves at processor `+0xB8`.
- The meaning of processor `+0x18C`, which the token and text paths set to 1.
- The writer of the tag output at processor `+0x200` and the pass that runs with `+0x5C` zero.
- Other modules that register the thunks `0x00B241C0`, `0x00CBFDC0` and `0x00B241B0`, and whether they treat `0x10`, `0x19` and `0x1A` the same way.
- How bold draws beyond the added advance of 1, and the role of bit 2 that code `0x1B` sets.
- The rendered result. No row break or shear was observed in a running client.
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
| Copied controls, entry and rows | `0x00592F30`, `0x00595A20`, `0x0098DAA0`, `0x0098D320`, `0x0098C3A0`, `0x00599170`, cell `0x01077CBC` |
| Copied controls, line fitting | `0x00988710`, `0x00988480`, `0x00987FC0`, `0x0055B3E0`, `0x00557810`, `0x00CBFE30`, `0x00CC2190`, `0x00CC0A40`, cells `0x00FA2B64`, `0x0110C1C4` |
| Copied controls, tokens | `0x00CC1C50`, `0x00CC14B0`, `0x00CBFD10`, `0x00B241C0`, `0x00B241B0`, `0x00CBFDC0`, `0x00B24220`, `0x00B241F0`, `0x00CC0C40`, `0x00CBFC90`, `0x00CBFC50`, `0x00CBFCD0`, `0x00CBFBD0`, `0x00CB2D50`, `0x00CC0670`, double `0x00FBF9D0`, vtables `0x0110BEFC`, `0x0110C014`, `0x0110C0BC`, `0x0110C35C`, `0x0110D95C`, strings `0x0110DBB4`, `0x0110DBBC`, `0x0110DC18`, `0x0110DC1C`, `0x0110DC20` |
| Copied controls, drawing | `0x00CAC330`, `0x00CAC700`, `0x00CAD710`, `0x00CADB50`, `0x00CA59E0`, `0x00CA54D0`, `0x00CB9BB0`, tables `0x0110C168`, `0x0110C408`, floats `0x0110BEB4`, `0x0110C314` |

For the copied controls, map each VA to a file offset through the section table, check each half-open range of the input tables against its SHA-256, and decode it with Capstone x86-32.
