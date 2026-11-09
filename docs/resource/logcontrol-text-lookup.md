# LogControl ordinary text lookup

## Input and method

Observed against FINAL FANTASY XIV 1.23b ffxivgame.exe, build 2012.09.19.0001,
15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9,
image base 0x00400000. All locators are VAs. Ghidra CLI 0.2.2 disassembly,
referenced tables and RTTI establish the static contract. Decompilation is
navigation only. Confidence is high for the cited instructions and data.
Conditional Lua producer frames and English row availability are recorded in
the [player producer contract](../script/logcontrol-player-producer.md).
Native lookup execution and a live result remain unproved.

The [pool feed contract](logcontrol-pool-feed.md) owns the native arguments,
category gates, listener, FIFO and publication behavior.

## Installed provider

OBSERVED. RaptureElementContainer constructs Component::Text::TextModule at
container +0x554 at 0x004DC05F..0x004DC068. Constructor 0x00CA0390 stores its
ExcelModule argument at module +0x30 and installs offset-zero interface table
0x01108F70. Complete-object locator 0x0119C754 and type descriptor 0x0130B8E8
identify .?AVTextModule@Text@Component@@. Interface slot +0x0C is 0x00CA0930.

OBSERVED. 0x004D7ACB..0x004D7ADA passes that module address to 0x0057A3C0.
The adapter constructed by 0x00771B80 stores it at adapter +0.
CoreWork constructor 0x00773270 stores the adapter at CoreWork +0xDC.
0x007634B0 wraps CoreWork using 0x00778D80 and installs the holder through
0x00CC9820 -> 0x00CD78E0 at underlying engine +0x220. Holder +4 contains
CoreWork. Table 0x00FDFFD0 and complete-object locator 0x0115FEE8 identify
.?AV?$Holder@PAVCoreWork@Client@Script@Lua@Application@@@AnyType@Misc@Sqex@@.
The reader at 0x007722CF..0x007722D6 reverses this chain. The ordinary adapter
call at 0x00771E14..0x00771E32 reaches provider slot +0x0C, concretely
0x00CA0930 on this installed chain.

## Owner key

OBSERVED. ActorBase constructor 0x006DBB70 initializes an owned UTF-8 object
at actor +0x08. Getter 0x00758860 copies it. Ordinary lookup 0x00771C20
requires a resolved owner and a nonempty copied key K.

OBSERVED. Registrar 0x0072E350 binds _loadTextDataPermanently to 0x006E7500.
The callback reads a numeric first cell and a string second cell. After a
successful named Excel lookup, 0x006E75CD..0x006E75D5 copies that string to
the native receiver +0x08. The receiver is saved at 0x006E7527. The deferred
branch captures the key and destination in TextDataReadResumeChecker,
constructed at 0x00712C80 with table 0x00FD4FD0. Callback 0x00718640 retries
the named lookup and copies the key to the captured destination when ready.
These observed writers establish a loaded sheet-name key. They do not prove
an actual K or an exhaustive writer inventory.

INFERRED. The
[conditional WorldMaster construction and key join](../script/logcontrol-player-producer.md#conditional-native-worldmaster-join)
connects the same native ActorBase +8 object to the published load39/worldMaster
path only under retained prototype/registry identity, the inferred table/self
relationship, selected method/callback, completed named loading, stable globals
and key, and valid lifetime. It selects no current instance or actual K.

## Sheet, row and column

OBSERVED. 0x00CA0930 receives TextModule in ECX and five stack arguments:
K, row selector R, column selector, parameter-set object P and output UTF-8
object O. It returns an integer and removes 0x14 stack bytes. This is an
assembly-observed calling contract. Ordinary 0x00771C20 supplies unchanged
R, literal 0xFF and initially empty O, and accepts only a result greater
than zero as completed text.

OBSERVED. 0x00CA0961..0x00CA0973 obtains ExcelModule from module +0x30 and
calls its +4 slot with K. Target 0x00C981D0 searches the UTF-8 name map at
ExcelModule +0x2C through 0x00800EC0 and 0x0099B920, using comparison helper
0x00446EF0. A miss returns zero. A found node supplies its sheet at node
+0x60 after a nonempty-vector guard on sheet +0x44. Registration 0x00C98580
uses the metadata name attribute as the same map key. Constructor 0x00CB04B0
installs ExcelSheet table 0x0110C6D4. Complete-object locator 0x0119D1C0
identifies .?AVExcelSheet@Excel@Component@@. Selection is by registered name.

OBSERVED. 0x00CA0982..0x00CA098F forwards R unchanged to sheet slot +0x18.
Concrete 0x00CAF760 delegates to the configured cache query and requires a
nonnull wrapper with ready row data. When sheet +0x3C is nonzero, its
alternate 0x00CAF6B0 also forwards R to cache +0x18 and checks readiness.
A miss can request loading and return zero. Helper 0x00C9A5F0 tests row-data
byte +4 against 1. This rule selects no cache mode or numeric R and promises
no synchronous completion.

OBSERVED. The 0xFF branch at 0x00CA09BA..0x00CA09CC calls sheet slot +0x1C.
Target 0x00CAF300 returns the byte at ExcelSheet +0x0B. Thus the log
call uses logical column L from the selected sheet's
[default configuration](#default-column-configuration).
Type helper 0x00CAF330 maps L through the byte array pointed to by sheet
+0x14, then indexes the type array pointed to by sheet +0x10. Parser
0x00CAF390 assigns str code 10. Jump-table index 9 at 0x00CA0DF8 sends that
type to 0x00CA09EC.

OBSERVED. The str branch obtains row data, calls 0x00C9A480 to locate the
four-byte cell using the row-data descriptor's column mapping, and reads its
UTF-8 object pointer. It forwards that object with P and O to TextModule slot
+8, then releases the row wrapper. Missing sheet or row returns zero.
Unsupported type returns -1. No bounds guard for malformed metadata is inferred.

## Default column configuration

OBSERVED. LLVM objdump 21.0.0 and original PE table reads establish the
constructor and lookup branches below. ExcelSheet constructor
`0x00CB04B0` initializes `+0x0B` to `0xFF` at `0x00CB04FC`.
It parses decimal `column_max` and `column_count` into bytes `+0x09`
and `+0x0A` at `0x00CB0597/0x00CB05B3`. It fills the
`column_max`-byte logical map at `+0x14` with `0xFF` at `0x00CB0664`
and allocates the `column_count`-byte type array at `+0x10`.

For each admitted `index` child, the loop parses its text in base 10 at
`0x00CB07FD`. At `0x00CB0808` it stores the zero-based physical ordinal
at the parsed logical index in the map. At `0x00CB080B..0x00CB0811`
it also stores the index's low byte as the default while `+0x0B`
still equals `0xFF`. The first declaration with a non-sentinel low
byte therefore seeds the default. The separate `type` loop at
`0x00CB075E..0x00CB0766` stores types in physical declaration order.
The selected row-data constructor retains the sheet at row-data `+0x0C`
at `0x00C9A82C`. The string cell accessor at `0x00C9A480` uses its map
to address row storage at `+0x08` plus four times the physical ordinal.
The index store and these lookup instructions have no separate logical
bounds or mapped-ordinal check. A valid lookup requires an in-range mapped
logical index and a physical ordinal below `column_count`.

OBSERVED. Registration reads `param` at `0x00C986B3`. The constructor
passes it to `0x00CAFA80` at `0x00CB08F2`. The parser splits the text
on the bracket characters at `0x00CAFAF0`. It splits each resulting
token on `|` at `0x00CAFBA8` and passes exactly two-token pairs to the
sheet `+0x20` map insertion at `0x00CAFC67`. It looks up `SHEET` at
`0x00CAFD6D` and splits the returned text on `;` at `0x00CAFD70`.
For the form `[SHEET|DEF;N]`, the second split gives `DEF` and `N`.
The first token must equal `DEF` byte for byte at `0x00CAFDD0`.
On the admitted two-or-more-token branch the parser converts the
second token in base 10 at `0x00CAFE0C` and stores its low byte at
`0x00CAFE14`. This store neither checks that the result is mapped
nor reserves the `0xFF` byte. This branch describes valid decimal
metadata. It supplies no validation rule for malformed text or indexes.

The [WorldMaster constructor boundary](../script/logcontrol-player-producer.md#native-default-and-execution-boundary)
records the initialized absent-attribute and missing-`SHEET` wrappers,
the empty-map condition and intervening callback paths. With absent
`param` and the normally initialized empty map at override entry,
both splits are empty and the override preserves the entry default.

### Installed SendLog owner definitions

OBSERVED. The three resource IDs follow the
[SendLog owner joins](../script/sendlog-sink-chain.md#sl-05-text-owner).
Decode their installed definition DATs with `decode_document` from
[xivl-tools at fd4c97d](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/tools/research/census_sheet_stack.py).
The decoded hashes below identify the complete XML documents. The
inverse document transform reproduced all three carriers.

| Named owner sheet | Definition | Carrier bytes | Carrier SHA-256 | Decoded bytes | Decoded SHA-256 |
| --- | --- | ---: | --- | ---: | --- |
| worldMaster | 0x03A20027 | 3489 | a33911281966c41711fc272fdbf070ab03cae717d47ce0a45a0b72acc0e74917 | 3488 | 2c69a2399cfaed999f90600777314355a552538ba6b7e0cd69f218e4de8f538f |
| harvestJudge | 0x03A2051C | 1694 | aa7b3743a1a524323904229b65a659d9283c087d7f7478ccf5a611cee922b94a | 1693 | 4e23a1c7ef79f377e4a3cce40844498cb95435777c9a6025019d65bacc8f949e |
| populaceGuildlevePublisher | 0x03A20153 | 1764 | 87f34d9e79d5b0e85fe9294964d2b91a3f78add112438583caa26cfe74077f80 | 1763 | 0f5ee10da200f441978d1abf16460536e55ceb03bbaf05ea2068e6ea791a3f24 |

Each named definition has one client-mode entry for each language shown,
with one physical `str` column and absent `param`. The table gives the
sole declared logical index L. Each entry has `column_count=1` and
`column_max=L+1`, and maps L to physical ordinal zero.

| Named owner sheet | ja | en | de | fr | chs |
| --- | ---: | ---: | ---: | ---: | ---: |
| worldMaster | 0 | 1 | 2 | 3 | 5 |
| harvestJudge | 0 | 1 | 2 | 3 | 4 |
| populaceGuildlevePublisher | 0 | 1 | 2 | 3 | 4 |

INFERRED. With matching English metadata admitted and the constructor's
seed and map retained through lookup, all three sheets select logical
1 and physical ordinal zero for the default request. This is the installed
configuration. Actual language, owner key, selected sheet, final native
default, row readiness and row text remain UNKNOWN.

## Conditional plain output

OBSERVED. TextModule slot +8, 0x00CA1D60, obtains the cell byte pointer and
forwards P and O through slot +0x24, 0x00CA1D40, to parser 0x00CB2760 at the
module +4 subobject. For a valid nonempty str cell with no byte 0x02 before
its terminating NUL, the ordinary branch at 0x00CB29B7..0x00CB2A72 copies the
pre-NUL bytes and appends them to O through 0x00447CF0. This branch does not
read P. The return at 0x00CB2A9D is normalized to integer 1 by 0x00CA1D40.
Since this caller initializes O empty at 0x00771DEC..0x00771DF3, that
conditional case returns the same pre-NUL byte sequence without parameter
substitution and meets the caller's positive-result gate.

UNKNOWN. No actual cell is established to satisfy this condition. Actual native
K/R, registered sheet, cache mode, final default logical or physical column,
and cell contents remain unproved. The player producer contract records exact
conditional call frames, a WorldMaster load callsite and absent English rows.
The [conditional native frame](logcontrol-pool-feed.md#conditional-four-explicit-native-frame)
and the producer's conditional WorldMaster join narrow the count/omission and
construction edges. Actual current globals, receiver/type and dynamic method
acceptance, same-instance/key continuity and lifetime, runtime language, category
admission, asynchronous completion, macro behavior outside this branch, fallback,
formatting and rendered visibility remain unknown. The rule does not remove the earlier
category 0x1B path or other gates in the pool feed contract.

## Reproduction

Use the identified executable in Ghidra. Trace the container construction,
adapter, CoreWork and holder stores and their reverse reader at the locators
above. Read TextModule table 0x01108F70, holder table 0x00FDFFD0 and their RTTI.
Inspect ActorBase 0x006DBB70, getter 0x00758860, registrar 0x0072E350,
callback 0x006E7500, checker constructor 0x00712C80 and retry 0x00718640.
Follow 0x00771C20 into 0x00CA0930, named lookup 0x00C981D0 and registration
0x00C98580. Read ExcelSheet table 0x0110C6D4, row paths 0x00CAF760 and
0x00CAF6B0, readiness 0x00C9A5F0, default 0x00CAF300, type mapping 0x00CAF330,
type parser 0x00CAF390 and branch table 0x00CA0DF8. Follow 0x00C9A480,
0x00CA1D60, 0x00CA1D40 and 0x00CB2760, including ordinary-path continuation
0x00CB2951..0x00CB297C and append tail 0x00447D27..0x00447D36.
Compare instructions and referenced data with the identified PE. Static
agreement establishes this generic rule, not a live retail outcome.
