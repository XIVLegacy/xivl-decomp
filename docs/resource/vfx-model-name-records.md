# VFX model name records

Under the selected modern context, kind 6 parameter names and kind 7 sampler
names are NUL-terminated byte records. Their indexed offsets select the first
character directly. The allocated Name attribute object instead holds a
separate character pointer at +4. These are distinct layouts.

## Identity and method

The binary is retail FINAL FANTASY XIV 1.23b ffxivgame.exe, build
2012.09.19.0001, size 15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9.
Addresses are VA at PE32 image base 0x00400000. Subtract that base for RVA
and map through the original PE sections.

Ghidra disassembly through ghidra-cli supplies the selected instruction
boundaries and decoding. Instruction encodings, direct relative targets,
RTTI chains, vtable cells and dispatch data were independently compared
with the identified PE. This is not a second complete disassembly or a
live-client observation. No model resource values are selected here.

The [RTTI catalog](../../config/ffxivgame.rtti.json) and
[vtable inventory](../../config/ffxivgame.vtable_slots.jsonl) identify:

| Table VA | Exact mangled RTTI symbol | Selected methods |
| --- | --- | --- |
| 0x010D1354 | `.?AVName@VfxResourceBaseAttribute@VfxResourceFormat@Vfx@Engine@CDev@SQEX@@` | Size 0x00C06B00, Save 0x00C06B40, Load 0x00C06B90, conversion 0x00C06AF0. |
| 0x010D1508 | `.?AVNameLibrary@VfxResourceBaseAttribute@VfxResourceFormat@Vfx@Engine@CDev@SQEX@@` | Save 0x00C06D90, Load 0x00C082D0, count conversion 0x00C06D70, element allocation 0x00C083C0. |

Dispatch cells 0x00C0B3E0/0x00C0B3E4 select kind 6/7 libraries at
attribute object +0xF8/+0x104 through 0x00C0B35C/0x00C0B365. Constructor
0x00C0DDA9..0x00C0DE30 installs NameLibrary table 0x010D1508 at both
offsets. Cross-references locate these sites without proving complete
reference coverage. The [material contract](seasonal-model-material-bindings.md#material-and-texture-joins)
owns the enclosing modern route, count fields and fixed pointer bases.

## Name record and native object

Let S be the serialized Name record, A the allocated Name object,
Q=ptr32(0x0136E5F8) and V=u32(Q+0xC). Size
0x00C06B00..0x00C06B30 and Load 0x00C06B90..0x00C06C31 dereference Q
without a shown null guard and compare V unsigned with three. Stable
readable context, selected vtables and readable terminated storage are
conditions. V>=3 is not a universal tag, variant or profile validator.

For V>=3, Load starts at S+0 and copies bytes through the first NUL into
a stack temporary. Size scans ptr32(A+4) through NUL and returns the byte
length plus one, including the terminator. No explicit serialized length
or pointer prefix is read on that path. For V<3, Load skips four source
bytes and Size adds four. Those skipped bytes have no established length
meaning here.

The element-allocation window 0x00C08423..0x00C08455 requests eight bytes
per Name. Constructor 0x00C06A80..0x00C06A8F installs table 0x010D1354 at
A+0 and initializes ptr32(A+4)=0. Load requests byteLength+1 character
bytes, stores that allocation return at A+4 and forwards destination,
stack source and byteLength+1 to helper 0x009D4600. No null-allocation
check is shown. Constructor null is not an allocated empty string or an
established usable default for Size or Save.

Save 0x00C06B40..0x00C06B82 measures the terminated bytes at A+4 and
forwards destination, A+4 and byteLength+1 to the same helper. It sets AL=1
after ordinary helper return. The helper body is uninspected, so these
arguments do not establish its complete block-copy effect, overlap
handling or invalid-storage behavior. Save itself writes no prefix and
has no version gate. No symmetric V<3 writer follows.

The selected conversion slot 0x00C06AF0 returns without changing
characters. Load calls it once for low flag byte==1, while Save calls it
twice for low flag byte!=0. These predicates do not validate an endian
enumeration or an encoding. UTF-8, normalization and case policy remain
unestablished.

At 0x00C06B95..0x00C06BE2, Load reserves 0x200 stack bytes and begins the
temporary at current ESP+8 after two saved registers. Its loop scans until
NUL without a shown source-length, name-length or file-extent comparison.
The reservation does not establish a safe public maximum.

## Library count and record alignment

Let B be the NameLibrary payload base. Load 0x00C082D0..0x00C08375 reads
a full dword count at B+0 into native library+4, clears native pointer-array
field +8 and starts the first record at B+4. Low flag byte==1 invokes
count conversion. The selected 0x00C06D70..0x00C06D83 slot uses
0x00C06690..0x00C066B7 to reverse all four count bytes. Element loops
compare unsigned. No count-to-file-span validation follows.

After each Name Load, the library calls its Size method and advances the
cumulative offset. At 0x00C0833D..0x00C08369, only unsigned V>=3 aligns
that offset to four bytes. For ordinary nonnegative nonoverflow cursors,
the next offset is align4(previous+byteLength+1). Native arithmetic wraps
at 32 bits, and the actual alignment uses a signed-remainder sequence.
No arbitrary-overflow rounding rule follows. The skipped padding supplies
no required fill value.

Save 0x00C06D90..0x00C06E71 writes the full count dword, with count
conversion before and after that store for low flag byte!=0. It starts
record destinations at B+4 and aligns both the record cursor and supplied
offset unconditionally. The record cursor uses the signed-remainder
sequence, while the supplied offset uses its low two bits. The complete
shown Save body contains no padding-fill stores. Its alignment has no
matching Load version guard, so it does not prove legacy symmetry.

The partial allocator window 0x00C083C0..0x00C0846C rejects an already
nonzero native array pointer and requests wrapped count*4 pointer-array
bytes. Create flag byte==1 requests eight-byte Name objects. Individual
allocation failure produces a null entry. The incomplete normal return
and cleanup coverage do not establish array allocation success or safe
failure handling.

## Conditional indexed name pointers

Let R be the fixed indexed-model header, M the fixed Material, T=u32(M+4)
and I=ptr32(M+0x20). For an admitted parameter descriptor j,
0x00C19746..0x00C1978C reads raw word I[2*T+j]. The sampler loop at
0x00C199A3..0x00C19A30 runs for signed T>0, reads I[T+i] and uses a signed
continuation comparison. The enclosing material notation and preceding
ordinal setup remain conditions of these bounded windows.

| Lookup | Count | Ordinal base |
| --- | --- | --- |
| Kind 6, parameter name | u32(R+0x28) | u32(R+0x70) |
| Kind 7, sampler name | u32(R+0x2C) | u32(R+0x74) |

Raw word 0xFFFF is a sentinel. Other words are sign-extended to index x,
then encounter a nonzero-count test and unsigned count<x rejection. That
inner comparison does not reject equality. High-bit words become large
unsigned operands. No supported high-bit domain or independent lower-bound
check is established.

With nonzero count, a non-rejected x and nonzero
d=u32(R+0xA0+4*(base+x)), the returned pointer is R+d directly. All index,
offset and pointer arithmetic retains native 32-bit wrap. The successful
path reads no character pointer at record+4. Zero count, rejected index
or zero d instead produces pointer zero. A readable nonzero R+d whose
first byte is NUL is a distinct empty-string case compatible with the
modern reader.

The parameter sentinel retains null and skips the fixed literal
comparisons. A nonsentinel lookup failure reaches those comparisons with
zero and no shown null guard at 0x00C1978C..0x00C197CC. Sampler sentinel
and failure write null into the temporary list before the list and T are
forwarded to 0x00C12D40. Neither path establishes a safe failure fallback,
complete registration, current owner or shader success.

These selected reads and branches supply no general malformed-input
safety, file bounds, supported encoding or lifetime contract. Complete
allocation, registration, diagnostic and cleanup behavior remain
unqualified. No default value, current runtime owner or live result is
inferred from a name record.
