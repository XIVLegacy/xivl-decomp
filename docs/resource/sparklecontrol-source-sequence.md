# SparkleControl Source and Sequence

## Input and method

Observed against FINAL FANTASY XIV 1.23b ffxivgame.exe, build
2012.09.19.0001, 15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9,
image base 0x00400000. All locators are VAs. Ghidra CLI produced the
disassembly, xrefs and decompilation used to navigate the trace. Instructions,
branch operands, referenced tables and RTTI were checked against the identified
PE. Confidence is high for these static observations. Runtime selection and
rendering remain unproved.

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

## Guarded native cases and Source reader

OBSERVED. Handler 0x008CEC60 reads its local operation argument and uses an
unsigned upper bound of 0x28 before indexing table 0x008CF3B8. Entry
0x008CF3F4 selects case 0x008CF0F5 for local 0x0F. Entry 0x008CF3F8 selects
case 0x008CF144 for local 0x10. With their holder/state nonnull, both cases
copy 0x19 dwords from the supplied payload into stack storage. Source reaches
0x008E2F40 through holder helper 0x008CC0B0 and helper 0x0060B270. Sequence
uses existing state +0xD8 and reaches 0x008E3CF0. Matching local ids and
100-byte shapes do not establish the ScenePacket-to-handler delivery bridge.

OBSERVED. RenderSparkle table 0x01066910 links to locator 0x01176F5C and type
descriptor 0x012DDB68,
.?AVRenderSparkle@Renderer@Window@Actor@Scene@Application@@. The store at
0x008E0B0B installs that table. DrawingToolContext, this renderer identity and
the native handler remain separate. The holder/materializer navigation does
not establish the complete allocation or queue bridge.

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

## Reproduction and limits

Use Ghidra CLI on the identified executable. Inspect the constructor,
property initializers, handler and apply targets above. Follow the getter,
interface constructors and factory, checking the cited virtual slots and
COL/type links. Inspect the staging methods, dirty consumer, submit helper,
ScenePacket constructor, native cases, Source loader and pointer/count reader
at their locators. Compare instructions, branch targets and referenced data
with PE section-based VA mapping. Decode the exact template basename with the
published SQEX algorithm and compare the stated package positions without
assigning other header roles.

UNKNOWN. The queue receiver at interface +0x20 and its delivery/translation
to 0x008CEC60, complete raw-file/cache identity, sequence selection through
0x008E3C30, full SPKL schema, signature/version validation, table bounds
policy and file record stride remain unproved. General property-helper flags,
staging-helper encoding/truncation/failure behavior, XML tag-to-factory
registration, actual runtime global selection, cached-object type, command
acceptance, marker-data ownership and rendered output also remain unknown.
