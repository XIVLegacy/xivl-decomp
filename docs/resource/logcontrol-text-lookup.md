# LogControl ordinary text lookup

## Input and method

Observed against FINAL FANTASY XIV 1.23b ffxivgame.exe, build 2012.09.19.0001,
15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9,
image base 0x00400000. All locators are VAs. Ghidra CLI 0.2.2 disassembly,
referenced tables and RTTI establish the static contract. Decompilation is
navigation only. Confidence is high for the cited instructions and data.
A real producer, selected row and live result remain unproved.

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
Target 0x00CAF300 returns the byte at ExcelSheet +0x0B, populated or overridden
from metadata. Thus the log call uses the selected sheet's configured default
logical column L. Its numeric value is unknown. Type helper 0x00CAF330 maps L
through the byte array pointed to by sheet +0x14, then indexes the type array
pointed to by sheet +0x10. Parser 0x00CAF390 assigns str code 10. Jump-table
index 9 at 0x00CA0DF8 sends that type to 0x00CA09EC.

OBSERVED. The str branch obtains row data, calls 0x00C9A480 to locate the
four-byte cell using the row-data descriptor's column mapping, and reads its
UTF-8 object pointer. It forwards that object with P and O to TextModule slot
+8, then releases the row wrapper. Missing sheet or row returns zero.
Unsupported type returns -1. No bounds guard for malformed metadata is inferred.

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

UNKNOWN. No actual cell is established to satisfy this condition. Actual K,
R, registered sheet, cache mode, default logical or physical column, cell type
and contents remain unproved. No producer or shared load callsite is joined
to this lookup. Parameter arity, four-argument Lua binder acceptance, category
admission, asynchronous completion, macro behavior outside this branch,
fallback, formatting and rendered visibility remain unknown. The rule does not
remove the earlier category 0x1B path or other gates in the pool feed contract.

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
