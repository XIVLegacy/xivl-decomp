# VFX model material values and texture requests

The selected version-4 B0 material and shader pair establish serialized component
counts and reflection targets. Native records read current shared value storage,
then issue conditional constant and sampler requests. The selected serial texture task reaches the
retained device interface. These are static contracts. They do not identify a
current B0 owner, complete omitted value lanes, successful device state or a
rendered result.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b ffxivgame.exe, build
2012.09.19.0001, size 15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9.
Addresses are VA at PE32 image base 0x00400000. Subtract that base for RVA and
map through the original PE section table.

Selected input: data/5D/1E/00/B0.DAT, resource 0x5D1E00B0, size 25420 bytes,
SHA-256 7dc48ec48b19e9e7bd1b6f5f50f056cb1b6275bcad3fb32d9914f907b8aaa38d.
File ranges below are decimal and half-open. The selected material record is at
1652. Vertex and pixel CTAB headers are at 20852 and 23356. Names and declared
registers describe this pair, not every VFX program.

The observations use Ghidra disassembly queried through ghidra-cli on the
analyzed ffxiv-1.23b project and ffxivgame.exe program. Decompiled output guided
navigation. Retained instruction bytes, direct relative targets, RTTI/table joins
and referenced data were independently compared with the identified executable.
Node Buffer reads and SHA-256 establish the selected source fields and ranges.
This method does not execute the client.

The [seasonal material contract](seasonal-model-material-bindings.md#material-and-texture-joins)
owns source-name and texture-ordinal joins. Its
[bbox and SHORT4 consumer](seasonal-model-material-bindings.md#texcoord5-normal-map-basis)
and the [WRB declaration contract](wrb-vertex-declaration.md) remain separate
canonical facts. The [predefined constant table](vfx-drawresource-and-shaders.md#engine-material-constants)
owns all 21 small/large-row definitions. No complete default table is repeated
here. This source material is distinct from a 0x2C-byte
[DrawResource override element](vfx-drawresource-and-shaders.md#material-element).
No actual or nonempty override list is selected.

## Serialized components versus current storage

The material has T=3 textures and N=22 parameter descriptors. Its eight-byte
descriptors occupy [1688,1864), sequential payload [1864,2004) and 28 u16 name
indices [2004,2060). Thirteen payloads are nonempty and total 140 bytes, SHA-256
351104392a7d7f624691b80d92f623198b977d621ea6311b0efad8b3e9085bb8.
Nine descriptors have zero length and classification 0xFF. These counts are not
a final native map size or proof of current value contents.

Each selected modern eight-byte descriptor uses byte +0 for the copied
component count, byte +1 for storage code, byte +2 for classification and
u32 little-endian +4 for declared payload length. The reads at
0x00C19727..0x00C1980B and 0x00C1991D zero-extend component/storage bytes
where used as counts/codes. The selected windows establish no read or
must-zero rule for byte +3. Every selected nonempty length is four times
its copied component count. The [material fixup contract](seasonal-model-material-bindings.md#material-and-texture-joins)
owns the count widths and pointer bases.

Attribute DataTable Load `0x00C06860..0x00C068A0` reads the same three
byte fields, then a full dword at C `+4` for unsigned context version>=3
or C `+3` below three. Save `0x00C06810..0x00C06853` always writes
the dword at C `+4` and leaves C `+3` unwritten. Zeroing allocated
object byte `+7` at `0x00C0E180..0x00C0E199` does not prescribe
serialized C `+3`.

The attribute Material Load cursor differs from in-place fixup and the
native-storage source copy below. Let c be descriptor byte 0, s byte 1
and L its declared dword length. At `0x00C0C595..0x00C0C68D` and
`0x00C0C8D3..0x00C0C9CB`, only c!=0, c!=5 and s==0 allocates,
copies and advances `4*c` bytes. L and classification do not size that
path. c==0 or c==5 explicitly stores a null payload pointer. When
c!=0/5 and s!=0, this path consumes zero bytes and does not write the
pointer slot. Its incoming allocation bytes remain unknown.

For stable readable modern context and ordinary nonoverflow traversal,
let `P=M+0x24+8*N`. Load's payload for admitted descriptor j starts at
`P+4*sum(c[k] for admitted k<j)`. Its final Index group starts at
`P+4*sum(c admitted)`, followed by `2*T` two-byte entries and N
two-byte entries at `0x00C0C693..0x00C0C72B`. Save instead copies
each full nonzero dword L and places its Index group at `P+sum(L)`,
then writes `2*T+N` entries at `0x00C0C261..0x00C0C302`.
Save `0x00C0C13D..0x00C0C1AF` writes a word at M `+0x12`:
`low16(0x24+low16(N)*low16(d)+sum(low16(L)))`, where d is the
DataTable serialized size, with word conversion when the low flag byte is
nonzero. In-place fixup consumes that word as signed16. GetSize
`0x00C0BFD0..0x00C0C099` instead gives the modern ordinary formula
`0x24+8*N+2*(2*T+N)+4*sum(c admitted)`. Native count, size and
pointer arithmetic retains the [allocation and traversal limits](seasonal-model-material-bindings.md#material-and-texture-joins).
These distinct cursors and narrowing operations do not validate their
agreement or the signed displacement for arbitrary records.

Load payload conversion compares the low flag byte to one and passes
values through FLD/FSTP and `0x00C066C0`. Save uses nonzero flag tests
around temporary conversion calls. These sequences do not prove a bit
permutation for every binary32 pattern under arbitrary x87 state. The
address windows do not establish complete called-helper behavior.

Length converter 0x00C029F0..0x00C02A27 changes only descriptor +4 when
its flag byte is nonzero. Fixup 0x00C024CA..0x00C0250D adds the converted
lengths with 32-bit wrap and logically shifts the aggregate right by two
for its payload iteration count. It passes payload values through x87
loads/stores and the conversion helper 0x00C02830. This does not prove a
pure bit permutation for every binary32 pattern or arbitrary x87 state.
The inspected 0x00C02830..0x00C02863 window stops before its complete
return. No divisibility, per-descriptor padding or file-bound check follows.

At 0x00C12F20, registration forwards automatic index -1 to 0x00C12B30.
Its successful new-key branch reserves ceil(sizeBytes/16) blocks, checks the
total against 256 and returns a metadata pointer. Helper 0x00C12F50 writes four
metadata bytes. Byte 0 selects the starting 16-byte block, and byte 1 encodes
supported sizes 4,8,12,16,64. These are registration writes, not source values.
The selected success return at 0x00C12C46 is bounded evidence. Complete
insertion/duplicate/error lifecycle and arbitrary malformed inputs are separate.

The selected source loop in 0x00C195D0 skips a zero-length descriptor unless
its classification byte is 0x36. Resolved names latitudeParam, lightCount and
ambientLightColor also skip at 0x00C1978E..0x00C197CC through comparisons
including their terminators. Failed name resolution is not a safe fallback.
At 0x00C198B9..0x00C1998C, let D be the retained DataTableMap and m the
successfully returned metadata. The destination is
ptr32(D+0x1C)+16*u8(m). For nonzero current length and an ordinary
nonnegative descriptor ordinal, 0x00C198EC..0x00C1991A divides the signed
32-bit wrapping sum of prior lengths by four toward zero, then scales it
by four and adds the payload base. Current length zero selects source
pointer zero. For the selected nonempty four-byte-aligned lengths, this
is the sequential payload prefix. It is not a general prefix or padding
rule for arbitrary lengths. The loop copies exactly descriptor byte 0
float components through FLD/FSTP. Its shown stores do not supply the
remaining lanes of a short value. Valid returned metadata, readable
storage, normal helper completion and suitable x87 state remain conditions.
Complete registration, diagnostic and error lifecycles remain unqualified.

These are the selected nonempty source-to-CTAB matches. Each target below is a
Float4-register set entry with declared register count 1:

| Source name | Supplied components | CTAB stage/register | Lanes omitted by the source copy |
| --- | ---: | --- | --- |
| ambientColor | 3 | Pixel c190 | W |
| exLightParam | 3 | Pixel c188 | W |
| normalPower | 1 | Pixel c191 | Y,Z,W |
| multiNormalPower | 1 | Pixel c192 | Y,Z,W |
| fogParam | 3 | Vertex c179 | W |
| fogColor | 4 | Vertex c180 | None |
| controlColor | 4 | Vertex c181 | None |

Six other nonempty names have no target in this selected CTAB pair. Conversely,
latitudeParam has no serialized payload but has pixel target c189. Its matching
name does not supply a value. File controlColor is not the evaluated effect color.
A CTAB match supplies a target, not a complete current Float4 or an upload.

## Value-area construction, reset and predefined conditions

The relevant retained table identities are:

| Table VA | Exact mangled RTTI symbol |
| --- | --- |
| 0x010B590C | `.?AVCore@RenderInterface@CDevVfx@Engine@CDev@SQEX@@` |
| 0x010B5C54 | `.?AVVertexShader@RenderInterface@CDevVfx@Engine@CDev@SQEX@@` |
| 0x010B5C90 | `.?AVPixelShader@RenderInterface@CDevVfx@Engine@CDev@SQEX@@` |
| 0x010D26E0 | `.?AVDataTableMap@Material@VfxResourceModel@Vfx@Engine@CDev@SQEX@@` |

DataTableMap table 0x010D26E0 slot +8 selects 0x00C12610. Construction
0x00C13D70 is admitted only when global ptr32(0x0136E740) is null. An already
present table skips that construction. For the selected capacity-256 embedded
layout, value pointer ptr32(D+0x1C) names 4096 bytes.
Construction at 0x00C13D70/0x00C12570 separates this area from a 32-byte bitset
and a 16384-byte name region. The explicit constructor clears target the bitset
and names, not the value area. Current allocation callback 0x01328D68 and
incoming allocation bytes, or an already-existing shared table, remain unknown.
No zero-value guarantee follows from the allocation kind argument.

The complete reset 0x00C12610..0x00C12638 calls 0x004169B0, clears D+0x28
and clears ceil(capacity/8) bytes at the separate bitset pointer D+0x20.
The complete 0x004169B0 writes only D+0x0C=0. Under valid selected layout,
normal called-helper completion and no concurrent or later writer:

V_after[0..4095] = V_before[0..4095], where V=ptr32(D+0x1C).

Reset preserves incoming/current value bytes. It does not create zero or default
material lanes. Name helper 0x00C127C0 writes at most 63 bytes and a NUL at
ptr32(D+0x24)+64*ordinal. Those stores do not initialize V either.

The canonical 0x00C13C20 predefined initialization runs only on its admitted
count-zero branch. Small rows of size at most 16 copy all 16 table bytes.
For fogParam, completed initialization before the B0 XYZ=(0,100,1) copy, with
no intervening or later value writer, preserves W=1. That conditional result
is (0,100,1,1). It is not an observation of current shared storage.

Size-64 rows take the calls at 0x00C13D38..0x00C13D49 instead of the
16-byte table-copy branch. Complete helper 0x00E3ACF0..0x00E3ACF5 returns
source address 0x0137B698. The caller passes metadata m from its registration
output local as the first stack argument, that source as the second and D in
ECX to 0x00BBB990. The complete helper 0x00BBB990..0x00BBB9F6 writes to
ptr32(D+0x1C)+16*u8(m), using 32-bit address arithmetic. Eight interleaved
loads and stores copy 8 bytes each at offsets 0,8,16,24,32,40,48,56, covering
all 64 destination bytes in increasing order. The four default floats in the row
are not this source. Destructive overlap can change later unread source chunks.

Complete wrapper 0x00F2E000..0x00F2E00B passes 0x0137B698 to complete
initializer 0x0042FCF0..0x0042FDBA. Its eight 8-byte stores cover all 64 bytes
and establish a 4x4 identity from positive zero and binary32 1 at 0x00F54F70.
The source occupies the virtual-only tail of the PE .data section, rather
than a file-backed matrix payload. Pointer 0x00F53CD8 names the wrapper.
The PE entry sequence 0x009D4BAA..0x009D4BB3 calls 0x009E4E6C, then jumps
to 0x009D49CA. The bounded startup prefix 0x009D49CA..0x009D4B56 calls
0x009D8F9A at 0x009D4B0F. In complete walker 0x009D8F9A..0x009D902B,
a zero result from precheck 0x009D8ECB admits an ascending walk of four-byte
pointers in [0x00F3E66C,0x00F53CE4). It skips null entries and indirectly calls
nonnull entries. Slot 0x00F53CD8 lies within that range and names the wrapper.

If the precheck returns zero, table and control state remain stable, memory
and frames remain valid, callbacks preserve the expected x86 nonvolatile
registers and stack conventions, and the selected calls return normally,
the wrapper is invoked before the one later transfer
0x009D4B3C -> 0x00403640. Four arguments derived from the image base, zero,
command-line tail and startup show value support a WinMain-compatible
interpretation of that transfer. No exact original function name is inferred.
A nonzero precheck can return without this walk. Startup's diagnostic call
can syntactically rejoin the application path if it returns, so reaching the
transfer alone does not prove the selected table was invoked. Earlier helpers
or callbacks may reach Vfx initialization first. Later callbacks or startup
work may change the source.

If that initializer completes first with valid storage, a valid stack frame,
no destructive alias and the checked constant value, it establishes source
identity. If the admitted size-64 path then receives valid metadata, readable
source and writable destination storage without address wrap, completes
normally and retains identity through all eight loads without destructive
aliases or writers, it establishes complete destination identity. No later or
concurrent writer may change that destination if the result is to persist.
These are sufficient conditions, not a current matrix observation. Ordering
before every 0x00C13C20 call, source identity survival, other writers and current
matrices remain unobserved. Ambient and exLight fourth
lanes and scalar normalPower/multiNormalPower tails remain incoming/current-state
bytes unless an independently established writer supplies them.

## Current value records, handles and constant counts

Application at 0x00C10E40..0x00C10F78 and 0x00C0F7F0..0x00C0F824 walks
these shader lists:

| Shader field | Count byte | Record stride | Consumer |
| --- | --- | ---: | --- |
| +0x8C, vertex values | +0x44 | 8 | 0x00C1D3A0 |
| +0x90, pixel vectors | +0x45 | 8 | 0x00C1D160 |
| +0x94, pixel matrices | +0x46 | 20 | 0x00C1D1A0 |
| +0x98, samplers | +0x47 | 12 | 0x00C1D460 |

The typed consumers read D from ptr32(0x0136E740). For a vector record R in
either stage, the actual source is the 16 bytes at ptr32(D+0x1C)+16*u8(R+4).
Vertex record byte +6 chooses a vector or four-block matrix method. The matrix
branch supplies four current blocks from that start and requires 64 readable
bytes. Pixel matrix records instead select four current source blocks
with byte +0x10, transpose them through the shared helper 0x00A41650 and submit
transposed block j for each nonzero handle at R+4*j, j=0..3. No current matrix
value or default is inferred.

The per-record predicates 0x00C0ED60/0x00C0ED70 return AL=0 with four-byte
argument cleanup. They do not suppress these typed requests. They are distinct
from the lower register caches. A valid nonzero handle H resolves the register
through the current wrapper+8 table at H*8-4. Typed table slots +0x2C and +0x34
have these selected routes:

| Interface | Vector method and request | Matrix method and request |
| --- | --- | --- |
| VertexShader 0x010B5C54 | 0x00B95910 -> 0x0041A7D0 -> 0x00422F90, count 1 | 0x00B956A0, count 4 through vertex cache |
| PixelShader 0x010B5C90 | 0x00B95950 -> 0x00420CF0 -> 0x00423090, count 1 | 0x00B95700, count 4 through pixel cache |

The pixel-matrix record loop's individual row requests use count 1, separately
from the typed four-block matrix method. 0x00C10600 diagnoses a zero pixel
handle and still forwards afterward. No safe zero-handle fallback is inferred.
Matching reflection, valid live handles, readable current values, retained typed
interfaces and normal completion remain predicates.

Core table 0x010B590C slot +0x1C selects 0x00B9F8A0, whose complete forwarding
body passes (register,source,count) unchanged to 0x0041BFC0 and 0x00422F90.
The existing [vertex float-cache contract](skeleton-output-float4-submission.md#forwarding-and-direct-device-interpretation)
and [pixel cache contract](../model-pixel-shadow.md#register-resolution-cache-and-direct-device-abi)
own suppression and the direct SetVertexShaderConstantF/SetPixelShaderConstantF
interpretations. Cache equality can suppress a backend call. Source lifetime,
current cache/backend identity and the separate
[immediate or queued delivery conditions](../actor/mesh-matrix-input.md#delivery-lifetime-and-admission)
remain necessary. A request is not an unconditional device write.

### Matrix and system producers

With supplied matrix M, 0x00C10670..0x00C10B97 makes these Core requests:

| Start | Native Float4 count | Source |
| ---: | ---: | --- |
| 0 | 4 | M |
| 4 | 4 | Transpose of the shared affine-inverse helper result for M |
| 12 | 4 | M times current block 0x01308520 |
| 8 | 4 | M times current block 0x013085E0 |

Dirty mask 0x04 can replace 0x013085E0 with the product of current blocks
0x01308520 and 0x01308560. Other dirty/nonzero-handle branches remain
conditional. Product order, affine inversion and transpose retain their
[shared math authority](model-draw-context-matrix-registers.md#apply-routine).
These current blocks are not invented view/projection defaults. Native start 4
uses count 4 even though this vertex CTAB's worldITMatrix declares only three
registers at c4. Request extent and shader consumption are different contracts.

System producers 0x00C102B0 and 0x00C10BA0 use nonzero handle guards, runtime
blocks 0x0136E610/0x0136E600, explicit local zero/one values and conditional
light arrays behind shader +0xA4/+0xA8. Their sources are separate from similarly
named serialized fields. Current blocks, selected light records and evaluated
values are not established by the B0 file.

#### Current matrix setters and derived storage

These observations use the executable identity and Ghidra disassembly method
above. The following VA spans are half-open. A prefix qualifies only the stated
instructions.

| Role | Start VA | Exclusive end VA | Extent |
| --- | --- | --- | --- |
| Setter for block A | 0x00BB99B0 | 0x00BB9A23 | Complete |
| Setter for block B | 0x00BB9A30 | 0x00BB9A61 | Complete |
| 64-byte copy helper | 0x00419B60 | 0x00419BBA | Complete |
| Selected-object source chain | 0x00BB9D30 | 0x00BB9D89 | Prefix |
| Context-object source chain | 0x00BF71A0 | 0x00BF71FA | Complete |
| Derived-block getter | 0x00BD8300 | 0x00BD833B | Complete |
| System-product source path | 0x00C10670 | 0x00C106D8 | Prefix |
| Vector times matrix helper | 0x0042EC50 | 0x0042EDAA | Complete |
| Matrix product helper | 0x0042EDB0 | 0x0042EE11 | Complete |
| Four-row assembly helper | 0x004201A0 | 0x00420200 | Complete |

Let A name the 64-byte block at 0x01308520 and B the block at 0x01308560.
Setter 0x00BB99B0 reads its caller pointer at entry ESP+4 and copies eight
qwords in ascending source order to A. It ORs byte 0x01308628 with 0x0D
between the final source read and final destination store. Setter 0x00BB9A30
passes its caller pointer to 0x00419B60 with ECX=B, copying 64 bytes, then
ORs the dirty byte with 0x16. It next reloads source +0x28, stores that dword
at 0x01308620 and stores zero bits at 0x01308624. The scalar is the later
source read, not an entry snapshot. Both masks set bit 0x04.

The 0x00BB9D30 prefix receives C in ECX and selects
O=ptr32(C+0x0C+4*((u8(C+0x14)-1)&1)). It calls O's virtual slots +0x0C
and +0x10 with ECX=O. The first returned pointer supplies setter A, and
the second supplies the B copy, dirty OR and scalar reload/stores above.
The complete 0x00BF71A0 instead takes stack argument P and selects
O=ptr32(ptr32(P)+0xF0). It retains both slot returns before those copies.
Its first pointer must survive the second virtual call until the A copy.
Both indirect targets and the object identities remain unknown.

Getter 0x00BD8300 tests dirty bit 0x04. When set, it calls 0x0042EDB0 with
ECX=B, first stack argument a temporary destination and second argument A.
It copies the result to 0x013085E0, then ANDs the dirty byte with 0xFB.
Both paths return 0x013085E0. The 0x00C10670 prefix repeats this conditional
recompute between products using its supplied M. It establishes no later
backend behavior.

For four contiguous Float4 rows, the helpers support the mathematical
row-layout interpretation A*B. The native MULPS operands are B row lanes
followed by broadcast A[i,k], giving p[k,j]=B[k,j]*A[i,k]. ADDPS accumulates
(((p[0,j]+p[1,j])+p[2,j])+p[3,j]). The matrix helper evaluates A rows in
order 3,2,1,0 into separate temporaries before 0x004201A0 assembles output
rows 0,1,2,3. Operand commutation, reassociation, fused arithmetic and fixed
MXCSR or NaN behavior are not established.

These paths require valid objects, readable source blocks of 64 bytes, stable
sources and normal completion. The scalar source must remain valid through its later
reload. No general alias, overlapping-copy, atomic publication or
concurrent-source guarantee follows. Current values, source ownership, current
B0 identity and execution remain unknown.

To reproduce, query the named spans with bounded ghidra-cli disassembly on the
identified PE32 program. Map their VAs through the original PE sections, compare
instruction bytes and recompute direct relative call targets. Trace ECX, stack
arguments, helper cleanup and the later source +0x28 reload. Check the packed
multiply operand order and sequential additions before interpreting the layout.

#### Conditional BaseCamera source table

These spans use the identified ffxivgame.exe, PE32 image base 0x00400000 and
Ghidra disassembly method above. Ranges are half-open. A normal body excludes
exception handlers and transitive callee effects.

| Role | Start VA | Exclusive end VA | Extent |
| --- | --- | --- | --- |
| Forwarding caller | 0x00BB9FD0 | 0x00BBA006 | Prefix |
| Pair-container construction | 0x00BB9A70 | 0x00BB9B1C | Normal body |
| Source-object construction | 0x00E3D4F0 | 0x00E3D53A | Normal body |
| First address getter | 0x00E3C780 | 0x00E3C787 | Complete leaf |
| Second address getter | 0x00E3C790 | 0x00E3C797 | Complete leaf |
| Initializer receiver | 0x00E3CC40 | 0x00E3CC70 | Prefix |
| Initializer virtual call | 0x00E3CDC1 | 0x00E3CDCA | Window |

In 0x00BB9A70, a nonzero result from helper 0x00416480 is passed as ECX
to 0x00E3D4F0. Its returned EAX is stored at both C+0x0C and C+0x10,
where C is the outer routine's receiver. The zero-result branch instead
stores zero in both fields. These stores precede later predicate-controlled
0x00E3C660 calls, whose effects remain unqualified.

At 0x00E3D51B, source construction stores table 0x0113E1F4 at the incoming
object pointer before calling initializer 0x00E3CC40. Table[-4] at
0x0113E1F0 points to complete object locator 0x011A891C. Its +0x0C pointer
selects TypeDescriptor 0x01322124, whose NUL-terminated name at +8 is
`.?AVBaseCamera@Camera@Common@Vfx@Engine@CDev@SQEX@@`.
This identifies the static table. Its slots +0x0C/+0x10 contain
0x00E3C780/0x00E3C790, complete leaves returning addresses O+0x84/O+0xC4
for incoming ECX=O. They do not load or validate matrix contents.

The initializer prefix saves incoming ECX in ESI. The separate late window
calls slot +4 of the table at then-current [ESI], with ECX=then-current ESI.
The intervening body and called-helper effects remain unqualified. In the
0x00BB9FD0 caller prefix, nonzero C and a nonzero low byte of the second
argument select the branch with the 0x00BB9D30 call using ECX=C, after calls
to 0x00BB9C20 and 0x00BB9B30. This does not identify that C with a particular
construction.

The join to the preceding forwarding contract is conditional on normal
returns, expected nonvolatile-register preservation and pointer/table
continuity through initialization, later helpers and both later virtual
dispatches. If O remains selected and table 0x0113E1F4 remains its table at
both dispatches, the getters supply O+0x84 and O+0xC4. Each requires a valid,
nonwrapping, readable and stable 64-byte source region under the preceding
copy, lifetime, alias and concurrency limits. Current selection, ownership
and numeric contents remain unknown.

To reproduce, authenticate the identified executable and disassemble only
the stated spans. Map instruction and RTTI VAs through the original PE
sections, compare bytes and recompute direct branch/call targets. Trace
receiver/result registers and stack cleanup, inspect table[-4], locator
+0x0C, descriptor+8 and both slot dwords, then decode the two LEA/RET leaves.
Keep the initializer windows separate when assessing receiver continuity.

## Sampler names, generated values and native records

Hash helper 0x00C126C0 starts h=0x01000193 and repeats
h=((h*0x01000193) XOR unsignedByte) modulo 2^32, stopping at NUL or 64 bytes.
That key is not a sampler ordinal. Retained typed lookup 0x00B957D0 and successful
register getter 0x00B957B0 use the reflection state. Name equality alone does not
select a current record or texture.

At 0x00C11870..0x00C118D3, the ordinary pixel name loop supplies kind 0, its
source-name loop ordinal and the reflection register to 0x00C1D340. The constructor
clears record +0 and stores name pointer +4, kind byte +8, ordinal byte +9,
stage byte +0x0A and special-name flag +0x0B. Final reconstruction at
0x00C11ED0 retains stride 12. The flag uses the bounded ASCII case-fold comparison
0x00C1D2E0 with cubeLightDiffuseMap and cubeReflectMap. It stops within 20 bytes
and succeeds when either input reaches NUL. This is not a universal exact-name
predicate.

Under the canonical B0 source-name order, matching reflection, successful typed
lookup/getter, normal construction and no later mutation, the selected records
and generated-name targets are:

| Source ordinal/name | Pixel sampler stage | Kind / special flag | TexColor target | TexMatrix target |
| --- | ---: | --- | --- | --- |
| 0 / `_sampler_00` | 0 | 0 / 0 | c193, count 1 | c171..c174, count 4 |
| 1 / `_sampler_01` | 1 | 0 / 0 | c194, count 1 | c175..c178, count 4 |
| 2 / `_sampler_02` | 2 | 0 / 0 | c195, count 1 | c179..c182, count 4 |

Each TexMatrix target is an array of four Float4 elements. Registration
0x00C12D40 reserves 64 bytes for each nameTexMatrix and 16 for each nameTexColor.
Instructions 0x00C12ECD..0x00C12EF7 explicitly write four ones only to TexColor.
The shown construction, reset and name stores do not supply complete TexMatrix
contents. Incoming bytes and later writers remain conditions. No zero or identity
matrix follows. The generated names are additional records, not extra serialized
payloads among B0's thirteen nonempty descriptors.

## Requested texture, effective object and selected device call

In the ordinary pixel branch of 0x00C1D460, a valid nonnegative ordinal below
material +0x48 and a nonzero +0x2C array select current texture pointer T[i].
H is ptr32(T[i]+0x14) when T[i] is nonzero and 0 otherwise. Separate direct-record
and shadow replacement branches are not substituted for this path. Its signed
comparison is not complete malformed-ordinal validation. Current textures,
object validity and pointer lifetime are still conditions.

For stage t, 0x00C0EFA0 compares H with state+0x10+4*t. Equality skips the Core
call. A difference stores requested H before calling Core+0x20(t,H). Retained
Core 0x010B590C slot +0x20 is 0x00B9F840, forwarding unchanged to 0x00418410.
That lower routine tests global 0x01328ED8 mask 0x04. A set bit stores and forwards
zero, otherwise it stores and forwards H. Let Z be that forwarded object value.

At 0x0041BEE0, effective object E is ptr32(0x01329870) when Z=0 and Z otherwise.
It reads ptr32(E+0x34) and forwards that value with t through 0x004231C0 and the
manager rooted at 0x0132987C. Thus requested H, effective E and the backend/device
texture pointer are different quantities. A cached zero is not proof of a null
texture binding. Readable retained objects and normal forwarding are required.

Backend byte offset +0x2C has retained targets 0x004387E0, 0x00437160 and
0x00433D80. Under the selected serial table 0x00F660F8, complete
0x004387E0..0x00438810 builds a 12-byte task: table 0x00F64940 at +0, stage at +4
and transformed texture pointer at +8. Task slot 1 is 0x00435780. The serial body
calls it directly with the device retained at backend+4.

The complete processor 0x00435780..0x004357DC calls device virtual byte offset
0x104, method index 65, with (device,t,task.transformedTexture). Native diagnostic
strings identify SetTextureTask::Process and SetTexture. These observations
support the conditional SetTexture interpretation for the retained serial device
route. They do not identify the current backend or execute another backend's task.

At 0x00435798, exactly result==0 bypasses the D3D diagnostic. Every nonzero
result enters that diagnostic path. If it returns normally, RET4 follows as well.
No actual HRESULT, diagnostic disposition or successful device state was observed.
A matching upper cached request is not proof that the device holds that value.
No general retry, rollback or lifecycle contract is supplied here. The other
texture backend bodies 0x00437160/0x00433D80 and queued execution remain outside
this selected serial proof.

## Indexed request and remaining boundary

The selected generic model request (4,0,0,457,0,864) reaches
0x0041CB10 -> 0x004232F0 -> backend slot 26. The
[published indexed mechanism](floor-model-render-path.md#indexed-submission)
qualifies its conditional DrawIndexedPrimitive interpretation with triangle-list
type 4 under the retained backend identities. It does not select an active B0
DrawResource, process a queue or establish a returned HRESULT.

This page establishes no current shared/native B0 instance, override list,
resource residency, lifetime, current force-zero/fallback state, cache/backend
selection, activation, evaluated shader result or rendered output. Current short
tails, complete generated TexMatrix and predefined size-64 matrix values and any
later writers remain explicit unknowns. The separate
[color-root contract](../actor/garuda-plume-color-root.md) and
[MatrixRoot mode-zero calculation](vfx-root-matrix-mode-zero.md) remain canonical
for those controls.

## Reproduction

On the identified analyzed program, use bounded ghidra disasm queries with the
FUN_ selector, instruction count and explicit project/program for the named
source-copy, typed consumer, sampler, forwarding, reset and processor windows.
Inspect only the stated complete or bounded paths. Read table[-4], its complete
object locator and type descriptor to confirm the exact RTTI names above.
Independently map instruction/data ranges through the original PE sections,
recompute direct relative targets and track stack arguments, source extents,
current-value pointers and device-call arguments.

For the size-64 predefined path, disassemble 0x00C13C20 with 92 instructions,
0x00E3ACF0 with 2, 0x00BBB990 with 24, 0x00F2E000 with 4 and 0x0042FCF0
with 38. Each count reaches the complete return of its named routine. Compare
the mapped PE bytes and direct call targets. Track the registration output local,
stack argument order, destination arithmetic and all eight ordered transfers.
Trace every initializer source lane before its store and read the binary32 1
at 0x00F54F70. Check pointer 0x00F53CD8 and the source's .data virtual/raw
extents separately.

For the startup join, read PE AddressOfEntryPoint. Disassemble the entry
sequence 0x009D4BAA with 2 instructions, bounded startup prefix 0x009D49CA
with 127, complete walker 0x009D8F9A with 50 and complete argument helper
0x009E4966 with 42. The startup prefix stops before exception handlers and
the remaining tail. Compare mapped bytes and recompute direct call/branch
targets. Trace the precheck result, unsigned table bounds, null skip, indirect
call and four-byte increment. Read the selected slot from the stated table
range. Check the nonzero-precheck return and diagnostic rejoin separately.
Resolve GetCommandLineA at IAT 0x00F3E31C and GetStartupInfoA at 0x00F3E324
through the PE import descriptors, lookup words and names. Trace their argument
sources through 0x009E4966 and the four pushes before 0x009D4B3C. Bounded
xrefs do not establish exhaustive callers or writers. This startup join does
not order the wrapper before every Vfx initialization or prove source survival.

From the identified B0, recover the material descriptors, sequential payload
lengths, 28 name indices and both selected CTAB tables. Check all 22 source-name
joins and the sampler names before associating them with runtime handles. Retain
the predefined small/large branch distinction and compare reset destinations
with the separate value/bitset/name regions. The canonical cache, shared math,
source-lifetime and indexed-delivery pages provide their own reproduction limits.
No file-derived value or static call route supplies a current runtime result.
