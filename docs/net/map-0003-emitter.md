# Map serverbound 0x0003 emitter

The native producer at VA 0x004E0320 constructs a 560-byte Zone game message
for record value 3. Its true application is 544 bytes. It initializes the
first 540 application bytes and copies four additional source bytes whose
values remain unknown. The immediate caller supplies the selector through
its fourth argument W. It does not force selector 1.

The [declarative catalog](../../config/map_0003_emitter.json) and
[verifier](../../tools/verify_map_0003_emitter.py) retain the coordinates,
source joins, complete-body digests and claim limits. These are static facts.
They do not establish packet delivery or server acceptance.

## Input and method

The input is retail 1.23b ffxivgame.exe, 15996808 bytes, PE32 image base
0x00400000, SHA256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9.
All addresses are VAs. GhidraCLI disassembly with explicit project/program
selection supplied the instruction and data observations. The query records
do not retain a tool version. Complete instruction coverage and original PE
byte agreement were checked independently. No client was executed.

| Complete body, half-open | Bytes | SHA256 |
| --- | ---: | --- |
| [0x004E0320,0x004E04D6) emitter | 438 | 4b56b6dcc89cc550bdcab2be9d9f938151219dac2f39474d03429b693b8e9f2c |
| [0x004D8160,0x004D82D1) caller | 369 | 252d230ead1f020eee7741f8838c5107558c7def46afe26698b8d46ee0176a3a |
| [0x004DD8B0,0x004DD92E) float helper | 126 | 61d5b323e73da453c3d1721823776c87b31ef8237e19e605eb0ab8527000ca56 |
| [0x00DAE010,0x00DAE1D3) bridge | 451 | d3f7ce4b5b1f3a382c07066a46006b95a5312b1cb06da7b61085e3bba49797a3 |
| [0x0075BEF0,0x0075BFC5) upstream wrapper | 213 | 210f8d6e7023f20093f96ee332f8966f499441fe68f4ceee7b37e64cad791f69 |
| [0x006E91F0,0x006E9427) upstream dispatcher | 567 | c25e9ef201fab33ef27ba7a13defbad5d112c2ffddd34b0e32bd617d0e44d184 |
| [0x00763030,0x00763058) control-byte helper | 40 | e4a7ef572763cf5a91852bdc48ee84543025a6b73a3936469ea21063e8fff29a |
| [0x00445D60,0x00445DA1) byte comparator | 65 | a5baafd37b78fa9a166105f1a64516b7c59056c878a300413942c3c932a627c8 |

A digest authenticates the bounded body. The instruction/data interpretation
below supplies the semantic argument.

## Record, header and application

Let S be producer ESP after 0x004E0352 and R=S+0x84. At 0x004E0483 one
pending push makes the record pointer expression resolve to R. The three
uncleaned copy arguments hold ESP=S-12 during the explicit record stores.
Stores at 0x004E042F and 0x004E043A put value 3 at R+0 and internal length
0x230 at R+4.

The state-3 branch of 0x00DAE010 passes those values to 0x00DC1C60. It uses
the final Zone builder table 0x01129AE8. The existing
[Zone header contract](zone-outbound-header.md) establishes that builder's
16-byte header, opcode write at D+2, preceding allocation clear and selected
finalizer preservation. It supplies the constructor-to-header binding for
this record value. The value-2 route and the separate
[Chat channel](chat-outbound-header.md) retain their own contracts.

At 0x00DAE0A9..0x00DAE0B8 the bridge copies length-0x10 bytes from R+0x18
to D+0x10. For length 0x230 this is 0x220 = 544 application bytes.
The internal length includes the 16-byte game header. R+8..R+0x17 is not
copied as a source header. The destination header is constructed separately.

The published capture finding is
xivl-captures:studies/m9-corpus-research/derived/verdicts.md,
section Map main client-to-server 0x0003. Its two specimens have a 576-byte
wrapped sub-event and a 552-byte normalized view after an eight-byte inner
header. That view retains eight bytes of the 16-byte game header before the
actual application. Its selector/text/tail offsets +32/+36/+548 therefore
map to application +0x18/+0x1C/+0x21C. Its 32-byte prefix includes 24 true
application bytes. This reconciles coordinates and does not add a captured
specimen or change its observed bytes.

## Complete application write map

Application ranges below are half-open and relative to A=D+0x10. The matching
source region starts at R+0x18.

| A range | Source | Producer locator |
| --- | --- | --- |
| [0,4) | zero u32 | 0x004E0409 |
| [4,8) | argument 1 u32 | 0x004E0373, 0x004E0410 |
| [8,12) | argument 2 pointee+0 dword | 0x004E037D, 0x004E0410 |
| [12,16) | argument 2 pointee+4 dword | 0x004E0387, 0x004E0445 |
| [16,20) | argument 2 pointee+8 dword | 0x004E0392, 0x004E0445 |
| [20,24) | argument 3 float bits | 0x004E03A6, 0x004E044E |
| [24,26) | low16 of argument 5 | 0x004E03FB, 0x004E0427 |
| [26,28) | zero u16 | 0x004E041F |
| [28,540) | 512 staged text bytes | 0x004E0455..0x004E0468 |
| [540,544) | no initialization by this bounded producer | complete producer body |

The explicit write union is [0,540). The final source four bytes occupy
S+0x2B8..S+0x2BB. Text construction uses a separate local string at S+0x2C.
The staging operations target S+0x924..S+0xB23. REP MOVSD writes
S+0xB8..S+0x2B7. These destinations do not initialize the source tail, and
the bridge uses that record region as a copy source, not a writable object.
These are bounded valid-pointer/helper observations, not a claim about
arbitrary malformed pointers or hidden global side effects.

The destination allocation clear does not determine the copied tail values.
The later full 544-byte copy overwrites that destination range with the
source bytes above. No zero value, padding meaning, required value or server
tolerance is established for those four bytes.

The producer stages and copies 512 text bytes. An in-field terminator is not
universal. Text processing policy, malformed-string safety and general
encoding validity remain outside this finding.

## Immediate caller and conditional selector

At 0x004D8246 the immediate caller first pushes W from [EBP+0x14], its fourth
argument, then a local string pointer. Literal 1 at 0x004D825A belongs to
0x004DD8B0. Both helper exits at 0x004DD921 and 0x004DD92B consume that
argument with RET4. The caller stores its float result at 0x004D8270 and
passes the five producer arguments at 0x004D8279:

| Producer argument | Bounded caller source |
| --- | --- |
| 1 | zero when B+0x98 is null, otherwise [[B+0x98]+0x88] |
| 2 | pointer to three local dwords from B+0x9C/+0xA0/+0xA4 |
| 3 | float result of 0x004DD8B0(B,1) |
| 4 | resulting local string record |
| 5 | W, caller argument 4 |

B is the nonnull cast-helper result checked at 0x004D81CF. Its fields'
population at this call, gameplay meanings, coordinate system and float units
remain unknown. The extra staged dword from B+0xA8 is not consumed by this producer.

The caller requires 0x00C99E40 to return nonzero AL after processing its text
input into the local record. When T+0x660 is nonnull it also calls
0x004CE760 with W, caller argument 2 and that record. No result gate follows
the optional call. The text passed onward is the resulting local record.
Its transformation and optional-helper effects remain unknown.

The selector is low16(W), followed by zero u16. The capture finding labels
value 1 Say. A separate conditional upstream call below supplies W=1. The
float-helper literal at 0x004D825A remains a different argument. The retained
direct-call references do not establish absence of indirect callers.

## Prefix source descriptors and getters

These observations use the same authenticated retail 1.23b `ffxivgame.exe`
and VA convention stated above. GhidraCLI disassembly and original PE data
supplied the findings. Complete instruction coverage and original byte
agreement were checked. No client was executed.

At 0x004D81B6..0x004D81C5 the call to 0x009DA6CC is supplied descriptor
addresses 0x01269C58 and 0x01269C88. Their names at descriptor+8 are
`.?AVRaptureElement@Main@Application@@` and
`.?AVRelativeElement@Element@Main@Application@@`. If the helper obeys the
MSVC `__RTDynamicCast` ABI, its checked nonnull result B is a pointer to an
`Application::Main::Element::RelativeElement` subobject, with RaptureElement
as the source descriptor type. The helper body and ABI are not independently
established here. A derived object could supply that subobject. B's exact
dynamic class and invocation remain unestablished.

Constructor 0x00534090 calls 0x004DAB50 at 0x005340CE. Stores at
0x005340E4/+EA install table 0x00FA0D28 at receiver+0 and 0x00FA0C04 at
receiver+8. Their preceding locator pointers lead to 0x0114494C and
0x011449A8. Both locators name descriptor 0x01269C88, with numeric receiver
offsets 0 and 8 respectively. Stores at 0x005340F1..0x0053411D clear
+0x94/+0x98, set float +0x9C/+0xA0/+0xA4/+0xAC to +0.0 and +0xA8 to
+1.0. This establishes constructor initialization, not current chat values
or a constructor invocation on the selected B.

The two qword copies at 0x004D823E..0x004D825E stage four dwords from
B+0x9C..+0xA8. The [producer argument contract](https://github.com/XIVLegacy/xivl-decomp/blob/06b3891b743832ab021190b3429d9b437acdba19/docs/net/map-0003-emitter.md#complete-application-write-map)
consumes the first three. Getter 0x004CFE30 instead copies all four dwords
when its low flag byte is zero or receiver+0x98 is null. Otherwise it
recurses on that pointee with flag 1 and adds four single-precision lanes
at 0x004CFED1. The chat caller does not invoke this additive getter.

Float helper 0x004DD8B0 tests the low flag byte. A zero low byte or null
receiver+0x98 returns receiver+0xAC without reduction at 0x004DD924. A
nonzero low byte and nonnull +0x98 select recursion through that pointee
with flag 1 at 0x004DD8C6. It adds the current +0xAC and rounds the sum
to single precision at 0x004DD8D1. Only this recursive path applies the
following reduction. The binary doubles at 0x00F62F48 and 0x00F62FE0 are
represented as p=3.1415927410125732 and t=6.2831854820251465.

For ordinary finite inputs whose adjusted quotient is convertible to signed
int32, the shown operations add p for a rounded sum >=0 or subtract p for
a sum <0, divide by t, truncate to signed int32, and subtract that integer
times t from the rounded sum. A final store/reload at 0x004DD919/+1D
rounds to single precision. This describes the bounded operation, not a
mathematical modulo or universal output interval. The constants establish
no observed gameplay unit. The [caller contract](https://github.com/XIVLegacy/xivl-decomp/blob/06b3891b743832ab021190b3429d9b437acdba19/docs/net/map-0003-emitter.md#immediate-caller-and-conditional-selector)
joins its literal helper flag 1 and float return separately from W.

The non-constructor +0x98 writer, pointee RTTI, child +0x88 meaning,
ownership, lifetime and recursion termination remain unknown. No writer
is joined to the exact chat B or its source-field population. Layout
compatibility establishes no such join or universal helper equivalence.
Coordinates, axes, handedness, rotation axis, gameplay units, active/local
owner identity, current prefix values, invocation and delivery remain
unestablished. Malformed pointers, cycles, faults, nonfinite or out-of-range
inputs, conversion overflow and FPU/MXCSR modes remain outside the ordinary
finite reduction description. These findings establish no prefix origin
policy.

The additional complete bodies are half-open ranges. These digests do not
extend the catalog verifier's asserted scope.

| Complete body | Bytes | SHA256 |
| --- | ---: | --- |
| [0x00534090,0x00534142) constructor | 178 | 2e971e987e1172757ca23ec29b87a9fee2c523b7ee35dba4fd78a6f0dc1b583f |
| [0x004CFE30,0x004CFF31) getter | 257 | 2603d28c6763b956468a9e1d54168fcc0f993850c1529588af6be9630fed2b26 |

To reproduce, authenticate the input above, map descriptor, locator and
constant addresses through the PE section table, and disassemble the
complete bodies. For the additional bodies, using the analyzed program:

```text
ghidra disasm 0x00534090 -n 44 --project ffxiv-1.23b --program ffxivgame.exe --json
ghidra disasm 0x004CFE30 -n 61 --project ffxiv-1.23b --program ffxivgame.exe --json
```

The caller and float-helper ranges and digests are in the input table above.

## Conditional upstream selector-one route

At 0x0075BFA0, wrapper 0x0075BEF0 conditionally calls 0x004D8160 with W=1.
Let S be its ESP after the prologue and U its incoming ECX. The prologue
saves 0x74 bytes, placing the fourth argument at S+0x84. The byte comparison
at 0x0075BF21 and JZ at 0x0075BF3A select 0x0075BF8F when that low byte is
zero. Intervening MOV instructions preserve the comparison flags.

On that branch the wrapper reads K through its first argument and calls
0x004D9970 with receiver T=[U] and K. Let R be that opaque return. Pushes at
0x0075BF9B/+9D/+9E/+9F supply literal 1, wrapper argument3, wrapper argument2
and R. The resulting 0x004D8160 arguments are R, argument2, argument3 and 1.
This call edge supports conditional selector-one construction if the existing
downstream cast/text/emitter/bridge gates pass. It establishes no runtime
occurrence or successful emission. The nonzero-control branch uses separate
unexpanded helpers and has no established overall emission policy.

Dispatcher 0x006E91F0 calls this wrapper at 0x006E92ED. Its selected branch
compares an opaque Q1 through 0x00445D60 at 0x006E92C4 against the literal
say at 0x00FD4BF0. The helper compares exact bytes through the terminating
zero and returns nonzero AL on equality. Equality falls through 0x006E92CB.
The literal identifies comparison bytes, not a gameplay action, UI/OS input
path or actual message text. No semantic function name follows from it.

Let C be the dispatcher's incoming ECX and P its explicit argument. The
bounded argument joins are:

| Wrapper argument | Dispatcher source |
| --- | --- |
| 1 | &K, with K=dword [P+0x0C] saved at stable S+0x18 |
| 2 | &L, the local object constructed by 0x00447200 using [C+0x60]+4 |
| 3 | Q0, an opaque 0x0071CE70 result for index 0 |
| 4 | EAX from 0x00763030, with only AL established |

Q1 is the same unexpanded helper's result for index 1. The helper's conversion
policy and all source contents/meanings remain unknown. Three pending pushes
make LEA [ESP+0x24] at 0x006E92E6 equal the saved K address S+0x18.

The dispatcher obtains opaque V from 0x00CC7510 with receiver [P+4], then
loads D=[[V]+4] and U=[D]. U is the wrapper receiver, and T=[U] is the
0x004D8160 receiver. These loads do not establish object types or population.
The selected 0x004D8160 arguments are R, &L, Q0 and 1. In particular, &L is
that caller's second argument and is not its producer's serialized context
u32 directly. R's selection algorithm and identity remain unresolved.

The control helper 0x00763030 receives D. Ordinary non-error paths return
AL=0 when dword [D+0x20] is zero. Otherwise they form H=dword [D+0x1C],
N=dword [H] and Z=dword [N+8]. A consistency case N==H calls unexpanded
0x009D22B4, whose behavior is unknown. With valid ordinary state, a null Z
returns zero AL and otherwise AL receives byte [Z+0xBA]. The source byte's
meaning, population and lifetime remain unknown. The dispatcher pushes EAX,
but the wrapper tests only its low byte. Upper EAX bits are not established.
Do not assign a semantic mode to either control case.

The v2 catalog separates upstream.staticConditionalCallProven=true from
upstream.runtimeInvocationObserved=false. The latter records the absence of
a retained runtime observation, not impossibility of invocation. The global
selector remains low16(W) with no fixed value. The verifier accepts v2 and
rejects the ambiguous legacy selector field. The outer wrapper and dispatcher
do not explicitly test the selected call result before cleanup/epilogue, so
this finding supplies no stronger return or delivery contract.

## Local results and verification

The caller requires T+0x174EC, a nonnull cast result and nonzero AL from
0x00C99E40. The producer requires its +0x234 object and that object's +0x70
receiver. The bridge accepts record value 3 only in its state-3 route, with
a nonnull 0x004E4C10 result and nonzero allocation result from 0x00DAF850.
It copies the application, calls 0x00DB06A0 and returns AL=1 at 0x00DAE0DF
without testing a finalizer return. The producer and immediate caller
preserve the bridge AL through local string destruction. These are local
normal-path results. Exceptions, faults, submission queue lifetime,
transport, delivery and server acceptance remain unresolved.

Run the asset-free catalog and synthetic checks with:

```text
python tools/verify_map_0003_emitter.py
python tools/test_promoted_static_checks.py
```

The verifier's optional --exe argument authenticates the exact input and
checks file-backed complete-body digests. It executes no client code.
Synthetic fixtures test repository validation and cannot supply retail
behavior evidence. The separate retail-input workflow retains its existing
fixed protocol-caller scope.
