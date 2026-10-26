# LogControl and pool feed static contract

## Input and method

Observed against FINAL FANTASY XIV 1.23b ffxivgame.exe, build 2012.09.19.0001, 15996808 bytes, SHA-256 9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9, image base 0x00400000. Locators below are VAs. Ghidra CLI 0.2.2 produced the disassembly, vtable data, RTTI walk, and decompilation used to navigate the trace. Instructions and data confirm the claims. Static analysis does not establish execution or live visibility.

## Document and shared module

OBSERVED. LogControl constructor 0x00595A20 allocates a LogDocument through 0x00592300 and stores it at control +0x1B04. LogDocument's primary vtable is 0x00FA951C. Its LogControlDocumentBase interface at document +0x28 uses vtable 0x00FA9504. RTTI type descriptor 0x012713A8 names .?AVLogDocument@CustomControl@SqwtInterface@Main@Application@@, with the interface offset recorded by complete-object locator 0x01149288.

OBSERVED. Per-frame 0x005954D0 calls accessor 0x004D7590 with control +0x1B08. The accessor returns dispatcher +0x660. The control caches that module pointer at +0x1B0C and passes it through document-interface slot 1, target 0x00593950, which stores it at full document +0xB0. Slot 2 is 0x00594A20. Slot 4, 0x00590960, returns the integer at full document +0x15C. The caller compares that integer with zero. The slot is not independently established as a boolean emptiness method.

OBSERVED. Document 0x00591F80 obtains line IDs and uses 0x004C3F10 to read category/source/text snapshots. It compares the recovered 16-bit category against 32-bit entries in the vector whose begin/end pointers are stored at control +0x1A5C and +0x1A60, and adds matching line IDs to the document's deque. No default category set is established.

## Native arguments and record

OBSERVED. _appendLogPool at 0x006ECF80 calls shared worker 0x006E8360 with flag 0. _appendMessagePool at 0x006ECED0 calls it with flag 1 after its yield request. The worker reads argument 1 as an integer category, argument 2 as an ActorBase control converted to identity, argument 3 as an integer text selector, and collects parameter cells beginning at index 4. Its logger is the second state-chain object +0xF8.

OBSERVED. Argument 0 selects source-name handling: integer -> 0x007721B0, UTF-8 string -> 0x00772560, WorldMaster -> 0x00772650, CharaBase -> 0x00772050. The integer and Chara paths resolve a source display name. The string path supplies that source name directly. WorldMaster supplies an empty source name. Argument 0 is not a literal message-body argument in these paths.

OBSERVED. The sinks allocate a 0x118-byte record at 0x0076B3D0/0x00776690 and append it to logger deque +0x08. It owns UTF-8 text at +0x00 and source name at +0x54. It stores argument 3 at +0x104, initially assigns the source integer/Chara ID or zero to +0x108, and stores the low 16 bits of argument 1 at +0x10E, source-name enable byte at +0x110, and flag at +0x111. The ordinary valid-owner branch at 0x007723CF..0x007723F0 resets +0x108 to zero before copying the source name. The field does not retain the initial source ID through that branch. WorldMaster sets source-name enable to 0. The other three sinks set it to 1. The ready byte +0x112, suppression/empty-text byte +0x113, fallback-result byte +0x114, and 16-bit poll count +0x10C initially contain zero.

## Conditional parameter-free path

OBSERVED. A bounded shared-worker case uses a WorldMaster source, category C, text-owner ActorBase A, text selector R, and no parameter cells at index 4 or later. The collector initializes an empty parameter vector when the cell count is 4. On the ordinary owner-lookup branch of 0x007722A0, the text owner must resolve, and 0x00758860 must return a nonempty key. That helper copies the UTF-8 object at actor +0x08. A conditional category 0x1B path at 0x007722E0..0x00772330 can set empty text, suppression, and readiness and return before owner lookup.

OBSERVED. On that ordinary owner-lookup branch, 0x007722A0 creates GetLogStringListener at 0x00777DF0 and calls 0x00771C20 on the object at second state-chain +0xDC. The ordinary lookup path resolves the owner again and invokes TextModule slot +0x0C, target 0x00CA0930, with the owner key, unchanged row selector R, literal 0xFF, a parameter-set object, and an output string. The [ordinary text lookup contract](logcontrol-text-lookup.md) establishes the installed provider, loaded sheet-name key, named Excel lookup, configured default logical column, and conditional plain-byte result. A result greater than zero forwards that output to listener callback 0x00777EC0. The callback copies the output to record +0x00, marks an empty output at +0x113, sets ready +0x112, and detaches the listener link. String assignment 0x00447450 copies into destination-owned storage.

UNKNOWN. The selected registered sheet for this instance, any numerically specified valid C/R pair, a concrete owner key, Lua binder omission rules, and the parameter or macro requirements of any real row remain unproved. The generic lookup rule establishes R as an unchanged row selector but selects no real row. This conditional native path is not a demonstrated parameter-free retail text or live-client feed.

## FIFO, flags, publication, and lifetime

OBSERVED. 0x00770C00 examines only the queue head. 0x0076F040 increments that record's poll count and returns its ready byte while the count is at most 900. Above 900 it takes a separate fallback path, sets +0x114, and returns success. This threshold counts polls. The ordinary ready head is removed, published through 0x00762F00, destroyed through 0x00758C20, and freed. An unready head prevents this function from processing later records.

OBSERVED. Publisher 0x00762F00 passes the record's zero-extended 16-bit category, source, text, and flag to dispatcher 0x004D6D90. A set suppression byte skips ordinary publication when flag=0. Flag=1 takes additional string-helper branches when suppressed. Their marker meaning is unknown. The dispatcher uses the same +0x660 module for both flags. Flag=0 forwards text directly. Flag=1 combines text with dispatcher +0x177B0 through 0x0044AB90 first. Separate message-yield or UI consequences are unknown.

OBSERVED. Module append 0x004CE760 serializes the category using %04X and combines category/source/text. The storage path 0x00C9A8E0 -> virtual +0x1C -> 0x00C9AD40 -> 0x00C9BAE0 copies the serialized bytes and advances line IDs. The append stores the returned ID in the category-specific deque at the vector-data pointer stored at module +0x50, plus C*0x14. The module is the same dispatcher +0x660 object returned by the document accessor. Pool records can be destroyed after publication because the module stores copied bytes. Reader 0x004C3F10 recovers the leading four hex digits as the 16-bit category and recovers source/text strings from the retained line.

OBSERVED. Record destruction clears the back-links of outstanding source-name and text listeners before destroying owned strings. This establishes those listener links' detachment, not complete asynchronous cancellation behavior.

OBSERVED. The _clearLogPool registrar 0x00739090 binds callback 0x006E1AD0. That callback calls 0x0075BD10 and logger 0x0076E100. The latter sets pending records' +0x113 bytes, preserving their pointers in the logger queue. It does not make every record ready or synchronously free every record. Marked flag-0 records are suppressed by the ordinary later publication gate. The first clear helper and effects on previously retained module lines remain unknown.

## Reproduction and limits

Use the identified executable in Ghidra. Inspect instructions at 0x00595A20, 0x00592300, 0x005954D0, 0x00593950, 0x006E8360, the four sinks, 0x00776690, 0x007722A0, 0x00771C20, 0x00777EC0, 0x00770C00, 0x0076F040, 0x00762F00, 0x004D6D90, 0x004CE760, 0x00C9AD40, 0x00C9BAE0, 0x004C3F10, and 0x0076E100. Read vtables 0x00FA9504 and 0x00F91434, and RTTI 0x01149288/0x012713A8. Account for the document interface receiver adjustment of 0x28. No instruction body, byte dump, or decompiled body is needed in a published artifact.

Category defaults, channel names, color mappings, actual sheet/default-column selection, row contents, complete LogFormat meaning, fallback output, scheduling, and live rendering are unknown.
