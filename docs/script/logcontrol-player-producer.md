# Player LogControl producers and English row availability

## Evidence and method

These are static observations from retail FINAL FANTASY XIV 1.23b inputs,
build 2012.09.19.0001. Lua offsets below are zero-based decoded-chunk byte
offsets, not original source lines. Native locators are VAs in ffxivgame.exe,
15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9,
image base 0x00400000. Ghidra CLI 0.2.2 instructions and referenced data
support the native joins. Bounded parsing of the bytecode and exact resources
supports the producer and row-availability facts. Decompilation is navigation
only. No script, native binder or client execution is established.

The [ordinary text lookup contract](../resource/logcontrol-text-lookup.md)
owns the installed provider and conditional plain-byte rule.

| Lua input | Bytes | SHA-256 |
| --- | --- | --- |
| PlayerBaseClass LPB | 25846 | 32182274f3886d0cdf6329f8a82e6592790c9b3038e00bebe5b2b38702bbd53e |
| PlayerBaseClass decoded chunk | 25833 | 1530dc2b53353888ccc6b47f78920ebd2c2d876335b9968196312153b0db464a |
| WorldMaster LPB | 1430 | da3adccea066061a6289b89af65bde92db13a5eabbdd1f0072565576d5414d20 |
| WorldMaster decoded chunk | 1417 | 6dca2434550518b09fe58d4878da3f0f0a77c1124808c231ad399b87089e420d |

The retail LPB paths are
client/script/729s9/uy9l5s/uy9l5s89r57y9rr.le.lpb and
client/script/nvsy6/nvsy6x9rq5s.le.lpb, respectively. The wrapper rule is
[XIVLegacy/xivl-client-structs:tools/decode_lpb.py](https://github.com/XIVLegacy/xivl-client-structs/blob/5bcd83fb29bf4f9493591b86dfce993f2680dea9/tools/decode_lpb.py#L46-L56) at
5bcd83fb29bf4f9493591b86dfce993f2680dea9. Both rle 0x0C inputs decode by
XORing bytes from offset 13 through the end with 0x73.

## Player call frames

OBSERVED. Root instructions at decoded offsets 872, 876 and 880 assign child
66 to PlayerBaseClass._onReceiveLimitAddicted on the existing global table.
Prototype 0.66 begins at 23694, with code at 23714. It has two fixed formal
parameters, no varargs or upvalues, and line-defined fields 8175 and 8202.
Per-instruction debug lines and local names are stripped.

OBSERVED. EQ/JMP guards at pc 0/1, 17/18, 34/35, 49/50 and 64/65 compare
formal R1 to literals 1 through 5. Each branch obtains _appendLogPool through
SELF from global desktopWidget. The receiver is desktopWidget. The explicit
source and text-owner arguments are separate reads of global worldMaster.
The category is literal 68. Formal R0 is neither source nor owner expression.

| R1 | CALL pc | Decoded offset | A/B/C | Selector R | Trailing explicit parameters |
| --- | --- | --- | --- | --- | --- |
| 1 | 9 | 23750 | 2/7/1 | 25252 | Integer 1 |
| 2 | 26 | 23818 | 2/7/1 | 25252 | Integer 2 |
| 3 | 42 | 23882 | 2/6/1 | 25253 | None |
| 4 | 57 | 23942 | 2/6/1 | 25254 | None |
| 5 | 72 | 24002 | 2/6/1 | 25255 | None |

SELF places the method in R2 and receiver in R3. Fixed CALL B=7 supplies
R3 through R8, including five explicit arguments after the receiver. B=6
supplies R3 through R7, exactly four explicit arguments. The last three
frames pass no trailing nil or stale R8. C=1 requests no return values.
These are fixed Lua call frames. The dynamic method value, native binder
acceptance and category 68 meaning or reader admission are unproved.

## Conditional WorldMaster load

OBSERVED. WorldMaster root offsets 92, 96 and 100 assign child 4 to
WorldMaster._onInit on the existing global table. Prototype 0.4 begins at
1045, with code at 1065. It has one fixed parameter and line-defined fields
261 and 269. After superclass initialization, SELF at child pc 3 obtains
_loadTextDataPermanently from formal R0. LOADK pc 4/5 supplies 39 and the
string worldMaster. CALL pc 6 at offset 1089 has A=1, B=4, C=1, supplying
the receiver and exactly those two explicit values.

OBSERVED. Native callback 0x006E7500 adds 0x03A20000 at 0x006E7568.
The literal 39 therefore requests resource 0x03A20027. Named-sheet readiness
allows the immediate writer at 0x006E75D5 or deferred checker 0x00718640 to
copy worldMaster into the receiver's ActorBase +0x08. This gives a conditional
key for an instance initialized through that method. It does not join the
player callback's global worldMaster reads to that native instance. Neither
chunk contains a direct SETGLOBAL for worldMaster or desktopWidget. This
bounded negative does not exclude indirect writes or other startup sources.

## Exact English resources

The resource format is described by XIVLegacy/xivl-tools:
[tools/research/census_sheet_stack.py](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/tools/research/census_sheet_stack.py#L100-L129) and [docs/formats/ssd-sheet.md](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/docs/formats/ssd-sheet.md#L81-L91) at
fd4c97d89eab3bbbf657021297e3d78156c63319. The exact resource identities are:

| Role | Resource | Bytes | SHA-256 |
| --- | --- | --- | --- |
| Metadata | 0x03A20027 | 3489 | a33911281966c41711fc272fdbf070ab03cae717d47ce0a45a0b72acc0e74917 |
| Data | 0x03A200BC | 128868 | 602e84634d394e4ae736f6c49f42c31b92a7fa6d5cae52cfe0c9519f07ec4982 |
| Enable | 0x03A200BD | 768 | 89f7af40e06ecefbb95f5d995f6b67a1ead3671d8dbe31407d29092aa0d3ca97 |
| Offsets | 0x03A200BE | 57324 | 1d6b168c4e8e03a68d347236957e5c5d8f52a58a266e8dee2179aefbde1ff8fc |

OBSERVED. Metadata data/03/A2/00/27.DAT decodes to 3488 bytes with SHA-256
2c69a2399cfaed999f90600777314355a552538ba6b7e0cd69f218e4de8f538f.
The document transform gives keys A=24416 and B=59431, agrees with independent
known-header checks and re-encodes exactly. The name=worldMaster, lang=en
definition declares column_max=2 and column_count=1. Its sole physical column
is str. Logical index 1 maps to physical column 0. It has no param attribute.
This identifies English metadata, not the language selected at runtime.

OBSERVED. The declared block has begin=20000 and count=14331. Its literal
fields select the data, enable and offset resources above. Resource selection
does not use neighboring filenames. All 14331 little-endian u32 end offsets
are monotonic and finish at data length 128868. The 96 nonoverlapping enable
ranges name 1449 rows, exactly those with nonzero spans. Sequential framing
of the sole str column reproduces all 1449 nonzero row boundaries. Length,
terminator and obfuscation round-trip checks agree without publishing text.

| Selector | Slot from 20000 | Enabled | Start offset | End offset | Span bytes |
| --- | --- | --- | --- | --- | --- |
| 25252 | 5252 | No | 43365 | 43365 | 0 |
| 25253 | 5253 | No | 43365 | 43365 | 0 |
| 25254 | 5254 | No | 43365 | 43365 | 0 |
| 25255 | 5255 | No | 43365 | 43365 | 0 |

The enable range 25251/count 1 is followed by 25256/count 44. All four
requested selectors are absent rows. A zero span is not a framed empty
string. String length/hash, token count, parameter-control classification
and no-0x02/plain-cell classification do not apply to an absent value.
The parameter-free Lua frames for 25253 through 25255 therefore do not prove
plain English cells. No replacement row or language follows from this result.

## Native default and execution boundary

OBSERVED. The parser used by 0x00C98A90 selects factory 0x00DC21E0.
The factory calls constructor 0x00DC3740, which installs table 0x01129BF0.
Complete-object
locator 0x011A318C and type descriptor 0x012E1214 identify
.?AVDOM_Element@Xml@Component@@. The parser passes that receiver through
child insertion 0x00DC34E0 and collection 0x00DC36F0 -> 0x00DC3590 to
registration 0x00C98580. Registration retains the receiver for its attribute
call at 0x00C986B3 and both ExcelSheet constructor callsites. Parsing,
allocation and vector helper completion remain conditions. This static join
does not establish a loaded native receiver.

OBSERVED. DOM_Element table slot +0x1C is 0x0099ADB0. An absent attribute
returns wrapper 0x0137B248 through the name search at 0x00DC2ED0.
Initializer 0x00F2DED0 calls 0x00445CF0, which gives that wrapper stored
size 1, cached length 0 and NUL content. Initializer 0x00F2C700 constructs
attribute-name wrapper 0x01376200 from literal param at 0x01108B90 through
0x00447260. Registration saves the attribute result at 0x00C986B7 and
passes it as the fourth explicit argument at both constructor callsites.
The constructor supplies it to 0x00CAFA80 at 0x00CB08F2. These wrapper
properties require normal static initialization.

OBSERVED. Constructor 0x00CB0811 initially stores the first logical index at
sheet +0x0B, giving 1 for this metadata. It also initializes the map at
sheet +0x20 with a self-linked sentinel at 0x00CB0511..0x00CB052C.
Before the override, block construction at 0x00CB08A8 calls 0x00CC3330
with the sheet pointer, retained at block +0x08 by 0x00CC3370. The enable
attribute can select resource-service slot +8 at 0x00CC3542 or +4 at
0x00CC354F with that block as callback and selector -2. Initializer
0x00F2CD80 anchors the attribute wrapper to literal enable at 0x0110DC88.
The selected service implementation, synchronous callback behavior and
writes through the retained sheet pointer remain unproved. Initial default 1
and the initialized map therefore do not establish their state at override
entry.

OBSERVED. The override initializes its local vector empty. Its first split at
0x00CAFAF0 reaches 0x0044F770 -> 0x00446FB0 -> 0x00446B10. A source
wrapper with stored size 1 returns 0xFFFFFFFF at 0x00446B22..0x00446B29.
The comparison at 0x0044F7B9 then exits before insertion 0x0044F7F9.
Thus the normally initialized missing-param wrapper leaves the first split
empty and skips the parameter insertion loop. Between the splits,
0x00CAFD15 calls 0x00444DC0 with equal zero range endpoints. Its branch
at 0x00444DE1 reaches 0x00444E2D without mutating the vector.

OBSERVED. The later SHEET query at 0x00CAFD6D uses ExcelSheet slot +0x24,
0x00CAF7B0, and the map at sheet +0x20. Search 0x00CB0B90 reaches
0x009130A0. On an empty map its sentinel test returns the miss iterator,
and 0x00CAF807 returns wrapper 0x01266B10. That file-backed wrapper has
pointer 0x01266B22, capacity 0x40, stored size 1, cached length 0, flags
1/1 and a pointed NUL byte. The second split at 0x00CAFD70 takes the same
no-insertion exit. The empty vector then takes 0x00CAFDA3 -> 0x00CAFE1B,
bypassing DEF comparison 0x00CAFDCB and the default-byte store at
0x00CAFE14.

INFERRED. With normal initialized wrappers and an empty sheet +0x20 map at
entry to 0x00CAFA80, absent param leaves both split results empty and
bypasses the local DEF store. On normal completion the override preserves its
entry default byte. If that byte is 1, it remains 1 through this local path. The
initial store at 0x00CB0811 does not independently prove an entry value of 1.

OBSERVED. The later constructor call at 0x00CB08FA reaches 0x00CB0140.
Its inspected instructions contain no direct store to sheet +0x0B. Its
row-block path reaches 0x00CC36D0 and 0x00CC2640, which can dispatch
resource-service calls with selectors -3 and -1. These later effects and
the earlier selector -2 effects leave map state, override-entry default
and the final native default unproved. No final physical column follows.
The row-absence result does not depend on those unresolved values.

UNKNOWN. Global native identities and startup assignments, invocation of
_onInit on the selected owner, readiness, dynamic method/native binder
acceptance, category 68 admission, actual language selection, native lookup
execution, asynchronous completion, fallback, formatting and rendered
visibility remain unproved. These producer and English asset facts establish
no usable plain-text feed or live retail outcome.

## Reproduction

Identify both LPBs by path, size and SHA-256, apply the cited wrapper transform
and identify the decoded chunks by SHA-256. Parse Lua 5.1 prototypes as data.
Use the root bindings, prototype IDs and decoded offsets above. Read each
instruction as little-endian u32: opcode is bits 0..5, A is bits 6..13,
C is bits 14..22 and B is bits 23..31. Decode constants and follow the
GETGLOBAL, SELF, LOADK and fixed CALL register origins, together with the
EQ/JMP guards. Compare CALL fields at the listed offsets. Decompiled output
line numbers do not identify original source lines.

Identify metadata and its exact declared English block by the resource table.
Decode the document with the cited format rules, preserving the exact decoded
hash. For selector R, slot i=R-20000 spans offset[i-1] through offset[i],
with slot 0 starting at zero. Read enable entries as little-endian u32
first-row/count pairs. The gap is visible at enable-file byte offsets 376
and 384. Compare all end offsets and enabled rows before framing strings.
Inspect the native load/base and default/override VAs above in the identified
PE. Keep the conditional instance join and final-default limits separate
from the exact absent-row result.
