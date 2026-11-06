# SparkleControl Source and Sequence

## Input and method

Observed against FINAL FANTASY XIV 1.23b ffxivgame.exe, build
2012.09.19.0001, 15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9,
image base 0x00400000. All locators are VAs. Ghidra CLI produced the
disassembly, xrefs and decompilation used to navigate the trace. Instructions,
branch operands, referenced tables and RTTI were checked against the identified
PE. LLVM objdump 21.0.0 and little-endian reads of the installed files verify
the ActionMenu sequence locators and offsets. Runtime selection and rendering
remain unproved.

## Property wrappers and dispatch

OBSERVED. Constructor 0x00990560 installs tables 0x0107801C, 0x01077EF4 and
0x01077EE0 at control offsets 0, 0xB4 and 0x194. Complete-object locators
0x0117EC94, 0x0117ECA8 and 0x0117ECBC identify those adjustments through type
descriptor 0x012BADA8, .?AVSparkleControl@Controls@Sqwt@@. The constructor
initializes the two wrappers with callback 0x009902D0 and stores class record
0x0135F7B4 at control +4 at 0x009906D6.

| Property | Info record | Id | Control wrapper | Setter | Apply target |
| --- | --- | --- | --- | --- | --- |
| Source | 0x0135F7F4 | 0 | +0x2A4 | 0x00990470 | 0x0098FE00 |
| Sequence | 0x0135F818 | 1 | +0x2AC | 0x00990250 | 0x00990280 |

OBSERVED. Initializers 0x00F26B40 and 0x00F26B60 bind the property names at
0x01077E5C and 0x01077E64 and ids 0 and 1. Both setters call generic helper
0x009BA4E0. Handler 0x009902D0 reads event +0x70, then descriptor +0x14, and
dispatches through table 0x0099037C. Id 0 reaches 0x009902F2 and calls
0x0098FE00 with the Source wrapper value. Id 1 reaches 0x00990305 and calls
0x00990280 with the Sequence wrapper value.

OBSERVED. Source apply zeroes control +0x2C0/+0x2C4. Its nonempty-input branch
calls 0x009A2270 and requires a nonnull result before reading unsigned 16-bit
values at returned-buffer +8/+0xA. At 0x0098FF0F..0x0098FF4D these become
binary32 values at control +0x2C0/+0x2C4. Conditional forwarding obtains an
object through 0x0093F880 and calls its virtual +0x60 slot. Sequence apply
0x00990280 similarly forwards through virtual +0x64 after its gate and nonnull
object check. These reads do not establish raw-file identity.

## Conditional DrawingToolContext path

OBSERVED. Getter 0x0093F880 checks 0x00974240, which reads global byte
0x0135C2B0. A nonzero gate returns null. Otherwise it can reuse control +0xA0
or call control virtual +0x54, storing the result at +0xA0. The common return
at 0x0093FAE2 loads that cached pointer. SparkleControl table 0x0107801C maps
slot +0x54 to 0x0093EF90. That method obtains the interface pointer through
0x00974220, global 0x0135C2AC, and with a nonnull interface calls virtual +4
using control +0xA8.

OBSERVED. Base constructor 0x009741F0 installs SqwtInterface table 0x01073574
and writes its receiver to global 0x0135C2AC. Constructor 0x0054E190 calls it
and installs table 0x00FA2D8C. Locator 0x01146664 and type descriptor
0x0126EB08, .?AVRaptureSqwtInterface@SqwtInterface@Main@Application@@,
identify that concrete interface. Its +4 slot is factory 0x005522D0.

OBSERVED. On its successful allocation path, the factory requests 0x10 bytes
and calls 0x00553D90, which installs table 0x00FA28E4. Locator 0x011459EC and
type descriptor 0x0126DFC8 identify
.?AVDrawingToolContext@SqwtInterface@Main@Application@@. The factory returns
this proxy only when its local-operation-0x24 call to 0x0054B0D0 returns
AL==1. Null allocation and other failure branches do not establish a proxy.
This constructed path does not type an arbitrary pre-existing cached object
or prove which runtime interface is selected.

| Property | Proxy slot | Target | Staging destination | Buffer +0x59 mask |
| --- | --- | --- | --- | --- |
| Source | +0x60 | 0x0055A260 | +0x27C | 0x08 |
| Sequence | +0x64 | 0x0055A2B0 | +0x2E0 | 0x10 |

OBSERVED. Both targets call 0x00559DE0. A null proxy +8 causes a 0x410-byte
allocation request and, on allocation success, construction through
0x0054D310. The result is cached at proxy +8. On the nonnull staging path,
the targets call 0x00445210 on the supplied value, pass destination/zero/100
to 0x009D2110, then destination/100/value-accessor-result/99 to 0x009DAB81.
They OR the distinct masks above into buffer +0x59 and set byte +0x5C to 1.
These are observed call arguments and writes. Encoding, truncation and
failure semantics of those helpers remain unknown.

## Dirty submission and code domains

OBSERVED. Consumer 0x0054ED88..0x0054EDDE tests the two dirty masks. Each path
requires global interface pointer 0x01336BA8, submits its staging destination
with size 100 through 0x0054B0D0, and clears its own mask only after AL==1.
A failure branches to 0x0054F0F2 before that clear.

The Sequence proxy at `0x0055A2B0` clears and rewrites the same 100-byte
field at `+0x2E0`. The consumer reads that field once per admitted submission
at `0x0054EDB8..0x0054EDDE`. If two sequence values reach the same staging
field before submission, the second replaces the first.

OBSERVED. At 0x0054B11F, helper 0x0054B0D0 adds 0x15 to its local operation.
It constructs an internal ScenePacket through 0x004EC080, submits through
the object at interface +0x20, virtual +0x0C, and returns that virtual call's
AL. The packet constructor writes a 16-bit code at +0x94 and a 16-bit payload
length at +0x96.

| Meaning | Local operation | ScenePacket code | Payload bytes |
| --- | --- | --- | --- |
| Source | 0x0F | 0x24 | 100 |
| Sequence | 0x10 | 0x25 | 100 |
| Proxy creation | 0x24 | 0x39 | 8 |

These are distinct internal code domains. Local operation 0x24 is not
ScenePacket code 0x24, and neither domain establishes a game network opcode.
The AL==1 branches do not prove runtime delivery.

### Ordinary Sqwt update and scene queue

STATIC. The [ordinary Sqwt update](sqwt-double-animation-clock.md#timer-dispatch-order-and-lifetime)
calls the timer wrapper at `0x005527C1`. After it returns, the update walks
the retained drawing proxies and calls `0x0054E890` at `0x00552841` when
proxy `+8` is nonnull. On an admitted visit that consumer first submits
proxy selection (local `0x22` at `0x0054E8FB`), then the dirty Source and
Sequence operations above, then dirty Percent (local `0x16` at
`0x0054EF28`). These are relative positions among the guarded submissions.

The [TimerIcon completion trigger](action-menu-timer-icon-style.md#observed-storyboard-completion-route)
requests `No_anime` and then `Effect_start` on the event source. On the
ordinary non-reentrant return path, both admitted writes to one proxy's
Sequence field occur before its flush. An admitted flush of that buffer
with no intervening overwrite submits `Effect_start` as its Sequence.
The setter forwards before calling its property notification helper at
`0x009C0321`. Alternate scheduling and reentry through notification
remain UNKNOWN.

STATIC. On the constructed interface path, `0x004B356E..0x004B3697` and
`0x004DC43C..0x004DC59B` supply the SceneModule retained at
RaptureSqwtInterface `+0x20` (`0x0054E207`). Its virtual `+0x0C` cell
`0x00F91DF4` selects `0x004E8390`, which calls the retry loop at
`0x004E8350`. The loop calls `0x004E82A0` to admit a copy into the
SceneThreadImpl ring at `+0x370`. The wrapper returns AL=1 at
`0x004E83A3` after the loop returns.

The scene job entry `0x004E9700`, registered at `0x004EAA15`, drains the
ring separately. It reads code from `+0x94` at `0x004E97F0`. The ordinary
route at `0x004E98D1..0x004E98F9` supplies that code, actor key from
`+0x90` and length from `+0x96` to `0x0060C140`, with the payload inline
at `+0x10` for length 100. The callee reaches keyed actor dispatch at
`0x007C93C0`. This establishes the
enqueue and consumer boundary. Which draw or present sees the resulting
state remains UNKNOWN.

## Guarded native cases and Source reader

STATIC. For the admitted WindowActor, dispatch at `0x007C95D3..0x007C95F2`
subtracts `0x15` and calls actor virtual `+0x274`. WindowActor cell
`0x00FF1F38` selects `0x007FA2B0`. With global `0x0134E06C` and a current
context at actor `+0x1A0`, the local `0x10` table entry at `0x007FA953`
selects index `0x0C`. Jump cell `0x007FA94C` reaches `0x007FA8EA`,
which calls the WindowContext dispatcher `0x008CEC60`.

OBSERVED. Handler 0x008CEC60 reads its local operation argument and uses an
unsigned upper bound of 0x28 before indexing table 0x008CF3B8. Entry
0x008CF3F4 selects case 0x008CF0F5 for local 0x0F. Entry 0x008CF3F8 selects
case 0x008CF144 for local 0x10. With their holder/state nonnull, both cases
copy 0x19 dwords from the supplied payload into stack storage. Source reaches
0x008E2F40 through holder helper 0x008CC0B0 and helper 0x0060B270. Sequence
uses existing state +0xD8 and reaches 0x008E3CF0.

OBSERVED. RenderSparkle table 0x01066910 links to locator 0x01176F5C and type
descriptor 0x012DDB68,
.?AVRenderSparkle@Renderer@Window@Actor@Scene@Application@@. The store at
0x008E0B0B installs that table. DrawingToolContext, this renderer identity and
the native handler remain separate. The full allocation and holder lifecycle
remain unresolved.

OBSERVED. On its reader path, Source loader 0x008E2F40 requires the
0x007FA1F0 resource result to
be nonnull with its +0x14 field nonzero. It stores the possibly null result
B of 0x009A2270 at receiver +0x6C at 0x008E2FF3, then checks B. On nonnull B
and successful 0x28-byte allocation, 0x008E303C calls 0x008E2E30 with B and index zero.

OBSERVED. Reader 0x008E2E30 uses nonnull B to select
P = B + u32LE(B + 0x14 + 4*i) at 0x008E2E70..0x008E2E78 and stores P at
reader +8. Null P skips the count/allocation loop. Otherwise the
unsigned word at P+4 bounds repeated 100-byte runtime allocation requests.
The loop latch at 0x008E2F03..0x008E2F17 rereads that word. The Source caller
uses index i=0. The allocation size is not a file record stride.

## Sequence name selection

OBSERVED. With a reader at renderer `+0x70`, `0x008E3CF0` passes the
sequence name to `0x008E3C30`. With section pointer `P` at reader `+8` and
a nonzero `u32LE(P + 8)`, the lookup sets `Q = P + u32LE(P + 8)`.
It scans up to `u16LE(Q)` entries in order. Entry `i` starts at
`E = Q + u32LE(Q + 4 + 4*i)`. The lookup compares the input and the
NUL-terminated name at `E + 4` byte for byte at `0x008E3C56..0x008E3C95`.
On a match it clears reader byte `+0`, clears the reader list at `+0x1C`,
and passes `u16LE(E)` to `0x008E35B0` at `0x008E3CD7`. That method stores
the selected index at reader `+4`. The renderer clears its fields at `+8`
and `+0xC` only if the lookup succeeds.

On the reader path at `0x008E3622..0x008E3677`, the selected index `j`
picks the record at `P + u32LE(P + 0xC + 4*j)`. The unsigned word at the
record start bounds the command loop. The first command starts at record
`+4`. The unsigned word at the command start advances the command pointer,
for example at `0x008E36A4` and `0x008E3931`.

## Template and package correlations

OBSERVED. client/sqwt/common/MapMarker.tpl, 15166 bytes, SHA-256
c9a9ddb8568e07bf67564e96328722c647a086ded15907900f73092c6706dba5,
decodes to 15158 XML bytes under the basename-keyed
[SQEX format](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/docs/formats/sqex.md#L35-L37).
Decode and re-encode reproduce the exact inputs. Its 16 SparkleControl
declarations use the three Source paths below and bind Sequence to XPath
@SparkleSequence in OneWay mode. This establishes declarations, not concrete
bound values, XML factory invocation or native instantiation.

The package identifiers below are relative to client/sqwt/. The raw-position
reads are in bounds for these inputs. They remain conditional correlations
because the complete raw-file/cache identity through 0x009A2270 and
0x009A1DB0 is unproved.

| Package | Bytes | SHA-256 | u16 pair +8/+0xA | u32 +0x14 | u16 at selected offset +4 |
| --- | --- | --- | --- | --- | --- |
| common/mapMarker.le.spk | 7016 | fda4bc86fc133aabffe460dff7370f22193c500d62a3e578992c153db3938c1b | 200, 200 | 472 | 1 |
| common/mapMarkerAetheryte.le.spk | 472 | 9622068b6edf47c445f556c45a5a888962584bad3f5ab40b5741c2e8dccc2a08 | 32, 32 | 48 | 1 |
| common/mapMarkerSymbol.le.spk | 8380 | 517c0ef62c909b1ce6b96507939446a3a522b844e25622932d765419ba92de04 | 32, 32 | 584 | 1 |

The selected offsets use index zero in the reader formula. Each selected
offset plus six bytes is in bounds. The values do not establish sequence
counts or record layouts.

The [MapScreen package-path boundary](https://github.com/XIVLegacy/xivl-client-structs/blob/5bcd83fb29bf4f9493591b86dfce993f2680dea9/manifests/s2c_018d_map_marker_presentation.json#L208-L211)
records callbacks 0x0067C220/0x00680810 and package literal 0x00FC3390.
Package-path use does not establish group_marker_data, MapMarkerParty or
wire-record ownership.

## ActionMenu completion package

The [ActionMenu TimerIcon style](action-menu-timer-icon-style.md#observed-form-and-style-declarations)
declares `common/timerEffect.le.spk` and the sequence names `No_anime`
and `Effect_start`. The named files in the build `2012.09.19.0001` install
are under `client/sqwt/`.

| File | Magic | Bytes | SHA-256 |
| --- | --- | --- | --- |
| `common/timerEffect.le.spk` | `SPKL` | 424 | `bcb3d8cf8fefce3bf77b0ab553226204f494153ed776458357b20ade4a8c1000` |
| `common/timerEffect.win32.gtex` | `GTEX` | 16416 | `9adacfe8e813660d427bcf2d712a8741c929fc5c89e55d25f2ada5edb1c47397` |

OBSERVED. In the SPK file, the unsigned words at `+8` and `+0xA` both
read 80. The index-zero section cell at `0x14` selects `P = 0x24`. The
unsigned word at `P + 4` reads 1. The dword at `P + 8` reads `0x30`, so
the sequence lookup formula selects `Q = 0x54`, where the entry count is 2.
The cells and records below apply those formulas to the file offsets.

| Sequence name | Table cell | Name entry | Selected index | Selected record | First command |
| --- | --- | --- | --- | --- | --- |
| `No_anime` | `0x58` | `0x60` | 0 | `0x84` | `0x88` |
| `Effect_start` | `0x5C` | `0x70` | 1 | `0x94` | `0x98` |

Each selected record contains one command whose leading unsigned word is
12. These offsets establish how the checked formulas would select the
named records if the renderer's package buffer holds this file.

OBSERVED. The Source loader takes the prefix before the first period at
`0x008E3072..0x008E30B0`. The declared source therefore supplies the stem
`common/timerEffect`. Helper `0x00933B80` qualifies the stem using runtime
path state. The loader passes `.gtex` and flag 1 to `0x00D39290` at
`0x008E3140..0x008E3179`. Its callee `0x00D38E50` adds `.*` at
`0x00D38F38` and the extension at `0x00D38FD6`. The resulting search
pattern uses `timerEffect.*.gtex`. When that probe fails,
`0x008E31A7..0x008E31E5` appends `.win32` and then `.gtex` to the
qualified stem. At `0x008E323B` the loader calls `0x00CA2590` with the
selected path. The installed sibling matches the search pattern and the
fallback name. The runtime root, probe result and texture resolution
remain unknown.

## OBSERVED ActionMenu completion render samples

The selected inputs are the installed timerEffect SPK and GTEX pinned in
[the completion package](#actionmenu-completion-package).
The GTEX is format index 4 (`D3DFMT_A8R8G8B8`), 64 by 64, depth 1,
flags 0 and one mip. Its offset table at `0x18` selects the 16384-byte
surface at data base `0x20`. These are checks of the installed files.
The runtime buffer and texture identity remain conditional.

### Clock and selected records

On successful construction, `0x008CC116..0x008CC137` constructs
RenderSparkle, stores it at WindowContext `+0xD8` and `+0xC8`, and sets
renderer byte `+4` to 1. The admitted WindowContext draw at
`0x008CEA7C..0x008CEAA8` visits that slot and calls renderer virtual
`+4`. Cell `0x01066914` selects `0x008E3A40`, which requires its
SPK buffer `+0x6C` and a nonnull named texture result from `0x00CA2190`
at `0x008E3A6B`. On the selected file, B-relative word `+0xC` is 30.
The loader divides integer 1000 by that word and stores 33 at renderer
`+0x74` (`0x008E3249..0x008E3255`).

A successful name selection clears the reader stop byte and renderer
deadline (`0x008E3CA1..0x008E3CD7`, `0x008E3D04..0x008E3D0F`).
On an admitted render with deadline zero, `0x008E3A7B..0x008E3A99` seeds
the deadline from `timeGetTime`. The due loop at
`0x008E3A99..0x008E3AD4` calls the reader `0x008E3600` once per due
step while the unsigned 64-bit deadline is at or before the zero-extended
clock reading. Each step adds 33 ms to that deadline. The renderer draws
after that loop at `0x008E3C12`. The first step can be due in the same draw that seeds
the clock. A later draw can process several steps before submitting a
quad.

The `Effect_start` lookup selects record 1. Its section count word at
`P+6` is 9, and the advance at `0x008E398B..0x008E39A2` visits records
1 through 8 on consecutive due steps. The table gives only the decoded
effective numerators consumed by this path. Omitted scale or alpha lanes
use the implicit default numerator 1024. Every sample command uses track
0 and selected section index 3 in words `+4/+6`.

| Record | Sample command offset | Mask at command +2 | X/Y scale numerator | X/Y position numerator | Alpha numerator |
| --- | --- | --- | --- | --- | --- |
| 1 | `0x98` | `0x04` | 1024 | 800 | 1024 |
| 2 | `0xA8` | `0x04` | 1024 | 800 | 1024 |
| 3 | `0xB8` | `0x05` | 1023 | 800 | 1024 |
| 4 | `0xCC` | `0x0D` | 1130 | 800 | 595 |
| 5 | `0xE8` | `0x0D` | 1199 | 800 | 320 |
| 6 | `0x104` | `0x0D` | 1244 | 800 | 140 |
| 7 | `0x120` | `0x0D` | 1271 | 799 | 35 |
| 8 | `0x148` | `0x0D` | 1280 | 800 | 0 |

The scale and color lanes divide signed words by 1024 and use 1 for
each omitted lane (`0x008E37E2..0x008E3823`,
`0x008E38AB..0x008E392C`). RGB is 1 throughout. Position divides each
signed word by 20 (`0x008E3863..0x008E38A6`). The constants are
`0x01050B00` (binary64 1/1024), `0x00FB6E90` (binary64 20) and
`0x00F54F70` (binary32 1). These paths store binary32 components.
Mask bit `0x02` is clear throughout and its transform terms are zero
(`0x008E3823..0x008E3863`).

In record 8 the preceding command at `0x13C` has mask `0x80` and
selector 1 at `+4`. Dispatch cell `0x008E3A20` selects `0x008E369D`,
which sets the reader stop byte and continues the same command loop.
The alpha-zero sample therefore applies in that step. The stop byte
bypasses future root command evaluation and suppresses record advance
(`0x008E3619..0x008E361C`, `0x008E398B..0x008E398E`).
Nested reader work can still run at `0x008E39A9..0x008E3A08`. The held
terminal state can reach draw submission with packed alpha zero. The
shared state flush requests alpha test GREATER 0 at
`0x00CA34AE..0x00CA34D7`, which rejects those fragments when active.

### Selected command and leaf fields

OBSERVED. For a type-2 reader section `P`, the track count is `u16LE(P+4)`
and the record count is `u16LE(P+6)` (`0x008E2E97..0x008E2F17`,
`0x008E3990..0x008E39A2`). The section-relative record offset table
starts at `P+0xC` and has four bytes per entry. At each record `R`,
`u16LE(R)` counts the commands and the first starts at `R+4`
(`0x008E3635..0x008E3677`). For a command `C`, `u16LE(C)` is its byte
length including the length word, and `u16LE(C+2)` is the mask. The
loop advances by that length at `0x008E36A4..0x008E36AF` or
`0x008E3931..0x008E3940`.

The sample branch reads the unsigned track and section indices at `C+4`
and `C+6` (`0x008E3785..0x008E37CA`). Its payload cursor starts at
`C+8` and advances through each present group in mask order. The
selected masks use these layouts, with all channel words signed and
little-endian. The lengths are the values in the selected file.

| Mask | Length | Channel words at command-relative offsets |
| --- | --- | --- |
| `0x00` | `8` | None |
| `0x04` | `12` | X and Y position at `+8/+0xA` |
| `0x05` | `16` | X and Y scale at `+8/+0xA`, then X and Y position at `+0xC/+0xE` |
| `0x0D` | `24` | Scale and position as in `0x05`, then R, G, B and A at `+0x10/+0x12/+0x14/+0x16` |

The cursor steps and signed reads are at `0x008E37E2..0x008E38A6`
and `0x008E38AB..0x008E3904`. Mask `0x00` is the nested record's
sample. The stop command with mask `0x80` is 12 bytes in this file.
Its selector is `u16LE(C+4)` and the selector-1 arm reads no channel
words (`0x008E367D..0x008E36AF`). The root has one command per record
through record 7, then a stop command followed by a sample in record 8.
The nested reader has one track, one record and one command.

OBSERVED. Let `S(i) = B + u32LE(B+0x14+4*i)`. `0x008E3318..0x008E337C`
dispatches on `u16LE(S(i))` and retains the type-1 section pointer for
leaf draws. The leaf count is `u16LE(S(i)+2)`. Entry `k` starts at
`L = S(i)+4+0x1C*k` (`0x008E1B24..0x008E1B57`,
`0x008E1C5C..0x008E1C76`). `u32LE(L)` selects a rectangle
section through `S`. The six little-endian binary32 words at `L+4`
through `L+0x18` are `sx, sy, x_to_y, y_to_x, tx, ty` in that order.
The stores at `0x008E1B5E..0x008E1BBC` populate an identity matrix so
the local transform is `(sx*x + y_to_x*y + tx, x_to_y*x + sy*y + ty)`.
The rectangle section supplies unsigned little-endian X, Y, width and
height at `+4/+6/+8/+0xA` (`0x008E1BD0..0x008E1C57`). Its leading word
is zero in the selected file.

For bounded extraction of this file, check each offset cell and referenced
span before following it. The four section cells occupy `[0x14, 0x24)`.
Their sorted destinations and the file end bound the root, rectangle,
leaf and nested reader spans at `[0x24, 0x160)`, `[0x160, 0x16C)`,
`[0x16C, 0x18C)` and `[0x18C, 0x1A8)`, respectively. The name offsets
use `Q` as their base and each selected name terminates within the root
section. The root and nested record tables fit in their sections, and
each counted command fits its record, ending by the next record or the
section end. The selected leaf count and stride fit its section and
its crop fits the associated GTEX surface. These are bounds for the
selected extraction, with retail malformed-input handling still unknown.

### Quad and default blend

The command path selects the track and sets its active bit at
`0x008E3785..0x008E37BA`. A changed section index reaches
`0x008E32E0` at `0x008E37CA`. That loader resolves
`B + u32LE(B + 0x14 + 4*i)`. Index 3 selects a type-2 nested reader
at file offset `0x18C`. Its one record at `0x19C` selects type-1
section index 2 at `0x16C`. The nested step occurs through
`0x008E39DE..0x008E3A08`. The type-1 section has one leaf entry,
selects section index 1 at `0x160`, and supplies the identity XY
scale with translation `(-32, -32)`. The selected rectangle words
at `0x164..0x16A` are `(0, 0, 64, 64)`.

Draw `0x008E1930` visits active tracks at `0x008E1968..0x008E1980`.
The consumer `0x008E19A0` composes the parent and track matrices through
`0x0042EDB0`, multiplies the track and parent color components, and
clamps the products to `[0, 1]`. It recurses for the nested reader at
`0x008E1B13`. The leaf applies its matrix at `0x008E1BC5..0x008E1BCB`
and submits the destination rectangle `(0, 0, 64, 64)`, source rectangle,
color and named texture at `0x008E1BD0..0x008E1C57`. In effect-local
coordinates, each axis of the quad is `t + s * (p - 32)`, where
`p` is 0 or 64, `s` is the decoded scale and `t` the decoded position.
For example, the first sample covers `[8, 72]` on each axis before
the renderer and window transforms. The outer renderer loads XY scale
from `0x012DCDD0` and applies optional translation from its
`+0x7C/+0x80` (`0x008E3AE0..0x008E3C07`).

The quad dispatch `0x00CA5760` selects `0x00CBC910` or `0x00CBAB20`
by global `0x013764A0`. Both normalize the source rectangle by the
texture dimensions and pack color through `0x00CB9640`, which clamps
and truncates each component times 255. With a white parent, the selected
vertex alpha bytes are `255, 255, 255, 148, 79, 34, 8, 0`. Both paths
select pixel program 1. Its bounded decode at `0x0110A680` multiplies
the sampled texel by interpolated vertex color. See
[the shared quad programs and device state](npcsay-backdrop-draw.md#states-and-programs).

RenderSparkle initializes blend byte `+0x78` to zero at `0x008E0B2D`
and forwards it through `0x00CA4170` at `0x008E3B00`. A local
operation `0x12` can change it through `0x008CF19E` and `0x008E0C10`.
With the constructor mode retained, `0x00CA38E8..0x00CA393A` enables
blending and supplies `SRCALPHA` and `INVSRCALPHA` to both the color
and alpha factor lanes, and `ADD` to both operation lanes. The factor
and operation wrappers are `0x0041D050` and `0x0041D010`. The
separate-alpha enable is state 206 set to 1 by `0x0041C480`, called at
`0x00A487A0` during device setup. With it enabled, a
source alpha `a` gives color `src_rgb*a + dst_rgb*(1-a)` and target
alpha `a*a + dst_a*(1-a)`.

### Projection boundary

These inputs support a bounded preview of the centered, expanding textured
quad and its recorded alpha fade using the installed texture and the
default blend. The selected WindowContext path still applies its actor
and state gates at `0x008CE7D9..0x008CE813` and its context color and
transform. The clipping helper `0x008CBEF0` forwards only with clip
substate `+0x15` nonzero. Substate `+0x14` equal to 1 requests disable.
When backend global `0x013764A0` is zero, `0x00CA3E30` uses the
rectangle and scissor route at `0x00CA3C60`. The other backend path
forces scissor disable.
Live command delivery, the runtime package and texture joins, window
placement and clipping, retained blend mode, draw cadence, backend
selection and pixel parity remain unresolved.

## Reproduction and limits

Use Ghidra CLI on the identified executable. Inspect the constructor,
property initializers, handler and apply targets above. Follow the getter,
interface constructors and factory, checking the cited virtual slots and
COL/type links. Inspect the staging methods, dirty consumer, submit helper,
ScenePacket constructor, native cases, Source loader, reader and sequence
lookup at their locators. Compare instructions, branch targets and referenced
data with PE section-based VA mapping. Decode the exact template basename with the
published SQEX algorithm and compare the stated package positions without
assigning other header roles.

UNKNOWN. Complete raw-file/cache identity, live ActionMenu sequence
delivery, reentry, full SPKL schema, signature/version validation, table bounds
policy and file record stride remain unproved. General property-helper flags,
staging-helper encoding/truncation/failure behavior, XML tag-to-factory
registration, actual runtime global selection, cached-object type, command
acceptance, marker-data ownership and rendered output also remain unknown.
