# Current recruitment member counts

Clientbound Map `0x01C8` supplies four current-count bytes, one for each
current recruitment details row. They are application/consumer-input offsets
`+0x1B`, `+0x27`, `+0x33` and `+0x3F`. The display loads them as unsigned
bytes. Input `+0x1E0` is the independent response-status byte.

## Offset and display contract

Let A be the application pointer passed to `FUN_004C9BB0`, O the operation
object, and i a zero-based row index from 0 through 3.

| Value | Consumer input A | Operation cache O | Width and interpretation |
| --- | --- | --- | --- |
| Current count | `+0x1B + 0x0C*i` | `+0xA1B + 0x0C*i` | u8, zero-extended for display |
| Requested count | `+0x1A + 0x0C*i` | `+0xA1A + 0x0C*i` | u8, zero-extended for display |
| Response status | `+0x1E0` | `+0xBE0` | Byte compared with 1; not a member count |

The existing application-dispatch contract in
`xivl-client-structs:manifests/search_recruitment_wire_contract.json`,
`packetBasis` and selector `0x01C8`, identifies A as IPC/game-message
`+0x10`. The first current byte is therefore IPC `+0x2B`. If an enclosing
subpacket begins with the separate 16-byte subpacket header, it is subpacket
`+0x3B`. These are equivalent offset bases, not three different fields or a
capture-observed packet extent. The outbound request's two u32 inputs in
[recruitment-1c8-caller-inputs.md](recruitment-1c8-caller-inputs.md) are a
separate direction and layout.

Native `FUN_004C8390` writes each current count to
`TextBlock_NumberOfPeople_1` through `_4`, property `IntData.Value2`.
It writes the requested count to the same controls' `IntData.Value3`.
`PcMatchingViewWidget.getParameter` reads those as user-work integer slots
3 (`NumberOfCurrent`) and 4 (`NumberOfPeople`). `displayData` passes current,
then requested, to UI text 228's two number macros, separated by ` / `.

For a nonzero requested count, current zero reaches the first number macro
unchanged: the static presentation is `number(0) / number(requested)`, with
no Lua-side leader addition, increment, blank substitution or zero-specific
hide branch. Requested zero hides the row. Purpose 99 separately hides the
first count control after formatting. Literal rendered glyphs were not
observed, and the native number-macro zero branch is not established here.
The later current-equals-requested test controls `TextBlock_Fixed_i`
visibility; it does not replace the displayed count.

## Native evidence

The input was retail 1.23b `ffxivgame.exe`, 15996808 bytes, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses below are VAs; RVA is VA minus that image base.
Ghidra 12.1.3 with `xivl-client-structs:ghidra/DumpVAs.java` produced
read-only decompilation and function metadata for `0x004C9BB0`,
`0x004C8390` and `0x004B6430`. The existing program was authenticated with
`ghidra/VerifyProgramFileBytes.java`: retained original/modified source
hashes and all seven initialized file-backed ranges matched the executable.
The two unbacked non-executable ranges were excluded. No program was saved.

Python 3.12, pefile 2024.8.26 and Capstone 5.0.7 independently mapped and
decoded original PE bytes. The following complete instruction ranges include
the function bodies; the first includes trailing alignment bytes.

| VA range, end exclusive | Bytes | SHA-256 |
| --- | ---: | --- |
| `[0x004C9BB0,0x004C9D60)` | 432 | `9cd038cb256fa81de1346ce1573c9059045f7d5928cf2b49c5a7ac0130e106f5` |
| `[0x004C8390,0x004C98ED)` | 5469 | `66c054b21ce514e6def5393cd98f3a91c2b40a6312f896911a4013cc9e20da7c` |

`0x004C9BFB..0x004C9C11` copies A+`0x1E0` to O+`0xBE0` and requires
status 1 for the details-copy branch. `0x004C9C36..0x004C9C73` copies four
12-byte entries starting A+`0x10` to O+`0xA10`, including each entry's four
trailing bytes. In that loop ECX begins at A+`0x14`, so `[ECX+7]` is the
first current byte A+`0x1B`. Both response branches set operation state 8
at `0x004C9D2D`; the non-1 branch emits text `0xB8B` instead of copying
details.

`0x004C8464..0x004C8473` selects operation state through the table at
`0x004C98F0`. Its state-8 cell at `0x004C9910` is `0x004C8565`, which
requires O+`0xBE0` equal to 1 before projecting the details.

| Row | Current `MOVZX` VA | Current cache byte | Requested `MOVZX` VA |
| ---: | --- | --- | --- |
| 1 | `0x004C8B13` | `+0xA1B` | `0x004C8B9E` |
| 2 | `0x004C8E50` | `+0xA27` | `0x004C8ED2` |
| 3 | `0x004C9181` | `+0xA33` | `0x004C920C` |
| 4 | `0x004C94C1` | `+0xA3F` | `0x004C954C` |

The first row's literal pairs are `IntData.Value2` at `0x00F8E5B8` with
`TextBlock_NumberOfPeople_1` at `0x00F8E5C8`, and `IntData.Value3` at
`0x00F8E5E4` with that control at `0x00F8E5F4`.
The calls at `0x004C8B2C` and `0x004C8BB7` pass the corresponding
zero-extended values to `FUN_004B6430`. Its 187-byte body builds a numeric
property value using `0x00454ED0` with radix 10 and passes it to
`0x00537DE0`. The projection finally writes `Button_Operate.IntData.Value2`
value 2 at `0x004C968D` and clears operation state at `0x004C96BB`.

## Authenticated Lua and text sources

The decoded Lua archive matched
`xivl-client-scripts:manifests/private_lua_corpus.json`: 14385427 bytes,
SHA-256 `0e8f902f7a2f592fc1220d41b89a3f35ec395cfb261806d4bd590a530099ae31`.
Selected source and original resource identities matched `scripts.json` and
`retail_lua_coverage.json` at revision
`0badc824adb353ca36ea41a61ee255a92a3231e2`.

`widget/pcmatchingviewwidget.lua` is 42975 bytes, 1828 lines, SHA-256
`29e75206c8e4de071b6a66bf69c779846e9153d0d19315c2c1685ca32e44cced`.
Its retail resource is `n1635q/u7x9q721w3o15nn1635q.le.lpb`, 13560 bytes,
SHA-256 `254a63575edfb08a3a7710c9c39f8801637135fa3852112b0cd72afac6708d48`.
`tools/retail_script.py:decode_lpb` produced the manifest-matched payload
SHA-256 `b6c2623faf2a591de48f7f8c80da0bde21e3ca892b1c2259d58d76f8836c7514`.
`tools/build_quest_contracts.py:ChunkReader` parsed the complete payload.

| Observation | Lua lines | Original prototype and zero-based PCs |
| --- | --- | --- |
| Requested/current getters use slots 4/3 | 959-980 | `root/proto4`, code `0x1BC0`, PCs 28-52 |
| Requested zero hides row | 590-618 | `root/proto3`, code `0xA67`, PCs 338-364 |
| Current/requested arguments to text 228 | 766-781 | `root/proto3`, PCs 504-517 |
| Equality and purpose-99 visibility | 782-828 | `root/proto3`, PCs 518-545 onward |
| PropertyChanged signal 2 calls displayData | 1449-1471 | `root/proto9`, code `0x2A45`, PCs 1-18 |

`xivl-client-data:manifests/tables.json` at revision
`f4bd2734570441a3721f460b85681435040b2724` pins
`csv/xtx__text_ui.csv`, 1284564 bytes, SHA-256
`23762a422118d99c4144a48b3495c52511b1b1b865e4fc50fdc61a973e3c6ece`.
Its authenticated CSV archive matched `manifests/private_csv_corpus.json`:
70110686 bytes, SHA-256
`006f9438a8cfd9277376f0ab28474500c67e4665050aa631cae64c9e6f38a5b0`.
Numeric columns 0 through 4 of row 228 all contain
`[@number:022003e80203] / [@number:022003e80303]`.

## Evidence boundary

This proves the retail client's stored unsigned values and static display
arguments. It does not prove whether the server includes a leader, which
members count toward each row, capacity policy, or any successful retained
`0x01C8` exchange. A literal `0 / N` glyph claim additionally requires the
native number-macro zero branch. The separate `0x01C7` list's row+`0x29`
`currentInt` must not be substituted for this details layout.
