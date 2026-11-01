# SendLog sink chain

The SendLog receiver turns the packets `0x0157` to `0x016A` into log records. This page records which record sinks its apply method calls, which record fields take the packet fields, how the category byte maps to the record category and where the record text is resolved. The receiver, its dispatcher chain and its parser inventory are in `XIVLegacy/xivl-client-structs:manifests/sendlogreceiver_demux.json` and `manifests/sendlog_parser_shape_refutation.json`. The sinks, the record and the publish path are in [LogControl and pool feed static contract](../resource/logcontrol-pool-feed.md). The speaker name template is in [LogControl Talker and the speaker display name](../resource/logcontrol-talker-name.md).

This page does not establish which sheet and row a SendLog selector names, the owner key of the lookup, the values of the category byte on the wire, or any line in a rendered log.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | `ghidra-cli 0.2.2` disassembly, cross-reference sets and decompilation as a lead |

All addresses are VA. Subtract `0x00400000` for RVA. Executable size and SHA-256 were checked first. Every claim was read from the disassembly, and call sites were enumerated with cross-references. This was static analysis, without client execution.

Evidence labels:

- STATIC: read directly from instructions or data at the cited address.
- INFERRED: a reading that joins static facts with a convention. The join is stated.
- UNKNOWN: not established.

## SL-01: Shared sinks

The SendLog apply method `0x0089FBF0` ends in a switch whose arms call the five record sinks. The pool worker `FUN_006E8360` calls four of them for `_appendLogPool` and `_appendMessagePool`. The fifth sink, `0x00771F50`, has no pool worker caller. STATIC, from the cross-reference sets of each sink.

| Sink | Calls from apply | Calls from the pool worker |
|---|---|---|
| `0x00772050` | `0x0089FF75` | `0x006E8599` |
| `0x00771F50` | `0x0089FED7`, `0x0089FF62` | none (other callers `0x00575C1C`, `0x00575DD9`, `0x00703E6A`, `0x00704625`) |
| `0x007721B0` | `0x0089FF9C` | `0x006E84E1` |
| `0x00772560` | `0x0089FFDD` | `0x006E8512` |
| `0x00772650` | `0x0089FE4E` | `0x006E855F` (other callers `0x00575F0F`, `0x006ED417`, `0x006ED501`) |

Every sink allocates its record with `FUN_0076B3D0` and calls the readiness poll `0x00770C00` (`0x00772024`, `0x00772176`, `0x00772278`, `0x00772612`, `0x00772707`), so a SendLog line enters the logger queue, the readiness poll and the publish step described in the pool feed page. It does not use the Lua pool natives. The pool feed page's statement that the sinks other than the WorldMaster sink set the source name enable to 1 covers `0x00772050`, `0x007721B0` and `0x00772560` and does not extend to `0x00771F50`, which passes 0 as the WorldMaster sink `0x00772650` does (SL-02).

## SL-02: Packet fields in the record

The apply method reads the receiver word at `+0x18` and the receiver byte at `+0x1A`. The constructor `0x0089FAAD` advances the packet pointer by `0x10` before the parser call (`0x0089FAB2`). Relative to that pointer the parsers write the receiver word from the word at offset 8 and the receiver byte from the byte at offset 0xA, except the parser of `0x0169`, which reads the dword at offset 0, the word at offset 4 and the byte at offset 6 (`sendlog_parser_shape_refutation.json`). The category byte is pushed unmapped to the sinks `0x00772650` (`0x0089FE39..0x0089FE4E`), `0x007721B0` (`0x0089FF91..0x0089FF9C`) and `0x00772560` (`0x0089FFC0..0x0089FFDD`). In the Chara arm and the display id arm the byte goes through `FUN_007642B0` (`0x0089FE8E..0x0089FE99`, `0x0089FF1A..0x0089FF24`) and the 16-bit result and the word are pushed to the sink `0x00771F50` or, in the display id arm, `0x00772050`. STATIC.

The sinks `0x00772050`, `0x00771F50` and `0x007721B0` call `FUN_0076B3D0(word, category, enable, flag)`. That function forwards its four arguments in order to the record constructor `FUN_00776690` (`0x0076B410..0x0076B426`), which stores them at record `+0x104` (dword), `+0x10E` (word), `+0x110` and `+0x111` (`0x007766FA..0x0077671E`). So the packet word is the record text selector and the category is the record category. The enable and the flag differ by sink:

| Sink | Enable at `+0x110` | Flag at `+0x111` |
|---|---|---|
| `0x00772050` | 1 (`PUSH 1` at `0x007720CB`) | its sixth argument, which apply passes as 0 (`0x0089FF6C`) |
| `0x007721B0` | 1 (`PUSH 1` at `0x007721E3`) | its sixth argument, which apply passes as 0 (`0x0089FF8A`) |
| `0x00771F50` | 0 (`PUSH 0` at `0x00771F7E`) | 0 (`PUSH 0` at `0x00771F80`) |

The sinks `0x00772560` and `0x00772650` push 1 and 0 for the enable (`0x007725B8`, `0x007726A1`). So a line that the Chara arm or the display id arm sends to `0x00771F50` has no source name enable, and the lines of the integer arm and of the display id arm that reaches `0x00772050` have it. STATIC, from the pushes and the argument slots of each sink. The pool feed page records the same fields for the pool natives (text selector at `+0x104`, the low 16 bits of the category argument at `+0x10E`).

`FUN_007642B0(value, context)` returns the value when it is outside `0x40..0x4F` (`0x007642B4..0x007642C6`). For `0x40..0x4F` it jumps to `FUN_00762AA0`, which compares the handle at context `+0` with the handle that `FUN_0057AB60` returns, and returns the value when they are equal and the value plus 1 otherwise (`0x00762AE4..0x00762B1A`). The second argument is a sender handle that the apply method builds with `FUN_00CC73B0` (`0x0089FE7D`) or reads from the variant at receiver `+8` through `FUN_00778D50` (`0x0089FF15`). INFERRED: the handle that `FUN_0057AB60` returns is the local player, so sixteen category values pair as a line from the local player and a line from another sender. The mapper applies only on the two arms named above, so the three unmapped routes keep the wire byte as the category. The wire values the server sends are not established here. STATIC for the arithmetic and for the arms.

## SL-03: Text resolution

Each sink that the apply method uses reaches the owner key lookup of `FUN_007722A0`. The sinks `0x00772560` and `0x00772650` call it at `0x0077260B` and `0x00772700`. The sinks `0x00771F50`, `0x00772050` and `0x007721B0` create the source display name resolver with `FUN_00789CD0` (`0x00771FCE`, `0x0077211A`, `0x00772223`), and the resolver callback `FUN_00789F40` (a slot at `0x00FE0470`) calls `FUN_007722A0` at `0x00789F6C`. STATIC. The lookup is the one in the pool feed page, which resolves the row selector in a sheet registered under the text owner key and reads the configured default column. The sheet and row that a SendLog selector names are UNKNOWN, because that page leaves the selected sheet and owner key open and the apply method does not name a sheet.

The speaker name does not come from this lookup. `FUN_0076FF20` calls `FUN_0076FD80` only when the byte at `+0x59` of the object at `[first argument + 0x60]` is zero (`0x0076FF55..0x0076FF5A`, `0x0076FFAE`). `FUN_0076FD80` takes the lookup path only when the byte at `[this + 0x14]` is nonzero (`0x0076FDC6`, `0x0076FDD1`). That path calls `FUN_0076FB10` for row `0xDE` of the sheet named `xtx/_text_ui` (the string at `0x0134BBC0`, built by `0x00F185B0` from the literal at `0x00FD87C4`, `0x0076FDFC..0x0076FE0A`). `FUN_0076FB10` calls `FUN_00769CC0` and `FUN_00769AE0` on the object at `[this + 8]` (`0x0076FB55`, `0x0076FBC5`), which write cells of the deque at that object's first field through the indexed accessor `FUN_008A68A0`. Whether that deque is the vector of the [Rich-text global parameter vector](../resource/rich-text-global-parameters.md) page is not established, and that page says nothing about a reset or load. STATIC for the guards and the call shape, UNKNOWN for the identity of the deque, INFERRED for the row as the speaker name template because the talker name page records that use.

## SL-04: What stays open

The text of a SendLog line, the category values on the wire, the owner key that the lookup uses for a SendLog record and the channel names are UNKNOWN. A client that receives these packets cannot reproduce a retail log line from this page. It can only store the selector, the category byte and the sender with the received parameters.

## Reproduction bounds

Verify the executable identity and PE32 base. Enumerate the cross-references of the five sinks and of `0x00789CD0`, then read the call setups in `0x0089FE20..0x0089FFFF`. Track each push and callee cleanup, because the decompiler lists several of these thiscall parameters incorrectly. Keep executable bytes, generated listings and function bodies outside published artifacts.

| Purpose | Ranges and data |
|---|---|
| Apply call sites | `0x0089FE20..0x0089FFFF`, `0x006E8360` (`0x006E84E1..0x006E8599`), constructor `0x0089FAAD` |
| Sinks | `0x00772050`, `0x00771F50`, `0x007721B0`, `0x00772560`, `0x00772650`, `0x00789CD0`, `0x00789F40` |
| Record | `0x0076B3D0..0x0076B42B`, `0x00776690..0x0077673E` |
| Category map | `0x007642B0..0x007642C6`, `0x00762AA0..0x00762B2D` |
| Speaker name lookup | `0x0076FF20` (`0x0076FF55..0x0076FF5A`), `0x0076FD80` (`0x0076FDC6`, `0x0076FDFC..0x0076FE0A`), `0x0076FB10` (`0x0076FB55`, `0x0076FBC5`), `0x00769CC0`, `0x00769AE0`, `0x00F185B0`, `0x00FD87C4` |
