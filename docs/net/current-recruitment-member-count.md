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

For a nonzero requested count N, current zero reaches the first number macro
unchanged. The [conditional ASCII construction](#conditional-ascii-construction)
produces `0 / N`, with N written as minimal decimal digits. There is no
Lua-side leader addition, increment, blank substitution or zero-specific
hide branch. Requested zero hides the row. Purpose 99 separately hides the
first count control after formatting. Actual runtime delivery and rendered
pixels are not established by this static construction.
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

## Conditional ASCII construction

For the constructor-selected TextModule/MacroDecoder tables, an admitted
TextBlock.Text reference path, the selected ready string row and normal
allocation, copy, CRT, locale and wrapper-helper completion, text 228
constructs current count, ` / ` and requested count as minimal decimal ASCII.
Thus current 0 and requested N in the visible-row domain `1..255` construct
`0 / N`. This is a conditional text-construction result, not an observed
runtime lookup or rendered display.

The original English row has the exact integer-reference frames below.
The [Sqwt argument and frame contracts](../resource/sqwt-plain-text-reference-lookup.md#sqtp-04-supplied-argument-selectors-and-output-handlers)
establish handler `0x20`, one-based `E8` supplied-integer references and
preservation of argument order. Its SQTP-05 subset requires each complete
frame's declared two-byte payload and closing byte.

| Row 228 component | Frame bytes | Selected value |
| --- | --- | --- |
| First number | `02 20 03 E8 02 03` | Supplied integer argument 1: current count |
| Separator | ASCII ` / ` | Copied literal text |
| Second number | `02 20 03 E8 03 03` | Supplied integer argument 2: requested count |

The source's [selected decimal conversion](../resource/sqwt-plain-text-reference-lookup.md#sqtp-06-selected-signed-decimal-conversion)
traces handler `0x20` through thunk VA `0x00A4B0F0`, constructor-selected
target `0x00C9F350` and `0x00454ED0` with constants 10 and 0. The selected
`%d` literal at `0x00F677E0` reaches `0x009D4F08 -> 0x009E51C4`.
Default precision one and width zero make zero one ASCII `0` digit on the
normal completion path; nonnegative counts have no sign, grouping or leading
zeroes. The linked source records its Ghidra CLI and independent LLVM/PE
instruction observations against the same executable identity above.

The Lua argument chain preserves both numbers. `WidgetBaseClass.setText`
calls `setItemText` with nil part selector, the control name and text ID.
`setItemText` supplies property `Text` to `setTextProperty`, whose numeric-ID
branch calls `_setTextProperty` with the same ordered trailing arguments
(`xivl-client-scripts:lua/scripts/widget/widgetbaseclass_common.lua:1833-1844,1859-1871,3276-3311`).
The source is 69077 bytes, SHA-256
`2057f6a4a94071bf8c9f2fa697bc0571b47da14a6bc4919a6d5491005417800f`,
matched against `manifests/scripts.json` with the archive identity above.
Its original resource `n1635q/n1635q89r57y9rr_7vxxvw.le.lpb` is 32996 bytes,
SHA-256 `c549f746370366f9827570ee22201516e07a05d91829ca643a2e2ec0c73bcf3c`;
decoded payload SHA-256 is
`7c45426b1a9f168557d84e420c5767c6fdea4abf90b976b276546af4e6a5de5f`.
Both matched `retail_lua_coverage.json`. The same decoder and ChunkReader
used above confirm open VARARG/CALL forwarding, with zero-based PCs:

| Method | Prototype | VARARG then CALL payload offsets |
| --- | --- | --- |
| `setText` | `root/proto89`, PCs 4/5 | `0x541A`, `0x541E` |
| `setItemText` | `root/proto91`, PCs 5/6 | `0x54D5`, `0x54D9` |
| `setTextProperty` numeric branch | `root/proto182`, PCs 12/13 | `0x7EDE`, `0x7EE2` |

The native widget serialization contract in
`xivl-client-structs:docs/widget-native-call-contract.md#property-access`
identifies `_setTextProperty` callback `0x006F5010` and handler `0x006F4910`.
A nonnil text ID builds `@id`; following integer arguments build `/i` plus
their value until the first Nil. Consequently these count arguments select
`@228/i0/iN` when current is zero. Zero does not terminate the argument scan.
The [conditional native property route](../resource/sqwt-plain-text-reference-lookup.md#sqtp-03-conditional-native-textblock-route)
retains Nil admission through the declared marshaller, input aliasing, TLS,
selected interface/tables, property-state gates, loading and lifetime as
conditions. The Lua call's nil part selector does not establish those
conditions for an actual invocation.

The [identified English backing strings](../resource/sqwt-plain-text-reference-lookup.md#uistr-01-selected-english-backing-strings)
pin definition resource `0x0B450082`, English sheet ordinal 1, physical str
column 0 and the block beginning at row 0. Python standard-library reads
authenticated these exact original resources before selecting row 228:

| Resource | Bytes | SHA-256 |
| --- | ---: | --- |
| Data `0x0B4502B5` | 71588 | `f99e738cf8762e0831f9675f37ddfea7b6072d475b3d4b0c9edeadb67bb013e0` |
| Enable `0x0B4502B6` | 1552 | `3f7c0b8461cfd18a336fb9bb72dbcd5439e65106eea8ea85a6ab731894866183` |
| Row offsets `0x0B4502B7` | 52080 | `c027d8115b2d9f7a67b0d1f1ec2b6623bdf214defd83f255a00dc96922ff2804` |

Row-offset entries 227 and 228 select data span `[911,930)`, a 19-byte
complete string frame with SHA-256
`a629f0d55f9120139d789b41fa9a3f39b8c1e8dbc67fc16f106435a59c0104be`.
The little-endian u16 declares 17 body bytes. Removing the `0xFF` marker
and `0x73` terminator and XOR-decoding the intervening bytes with `0x73`
produces the two six-byte frames and three-byte separator above, exactly
15 bytes, SHA-256
`eb7d65a2b3ac4939e26ffa4f08ffcfe3733365282f82b1cd0df9ce3bc32fd577`.
This pins the original English row rather than inferring frame semantics
from the CSV's macro label. It does not select a runtime language or prove
that a running instance loaded that row.

## Evidence boundary

This proves the retail client's stored unsigned values and static display
arguments. It does not prove whether the server includes a leader, which
members count toward each row, capacity policy, or any successful retained
`0x01C8` exchange. The conditional ASCII result does not establish actual
property admission, resource delivery or visible glyphs. The separate `0x01C7` list's row+`0x29`
`currentInt` must not be substituted for this details layout.
