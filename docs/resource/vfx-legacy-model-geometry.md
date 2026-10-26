# Legacy VFX model geometry

The selected legacy version-2.2 vfxm path has bounded vertex and index sources,
endian operations, numeric declaration mappings and converted part and material
framing. Its shader prefix passes record/header pointers separately from earlier
holder byte counts. These are static findings under valid storage, retained
object identity and normal helper-return conditions. They do not establish
successful native construction or a rendered result.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b ffxivgame.exe, size 15996808 bytes,
SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9.
All native addresses below are VA at PE32 image base 0x00400000. Subtract the
image base for RVA and map through the PE section table.

The observations use Ghidra disassembly queried through ghidra-cli 0.2.2.
Instructions, direct relative targets, table entries and RTTI were compared with
the identified executable. File fields and ranges were read with Node Buffer
operations and SHA-256, without executing client code. Matching bytes alone do
not validate instruction boundaries in adjacent functions or jump-table data.

The selected input is client/vfx/abl/0501, size 322820 bytes, SHA-256
427e062053f6b8da3bb3fc7c8d1104083505667c34871a6828020dcf0c0239b0.
Bank ranges below are decimal, half-open file offsets. No asset bytes or function
bodies are reproduced here. General framing is on the
[Qix container page](vfx-qix-resource-container.md).

## Selected container and version branch

| Selected structure | File range |
| --- | --- |
| Pack RES | [160,140122) |
| vmdl blob | [2080,7584) |
| Payload P | [2128,7584) |
| Root record window | [2240,2352) |
| Model chunk M | [2356,4174) |
| Shader set | [2312,2332) |

The root name is 4lbfwTpl0glw01w. Its type is 1, variant is 0, shader-set count
is 1 and animation pointer/size are zero. An instance chunk is present at
[4176,4497). Payload relocation mode is 2 with a clear relocated flag. The root
cell holds payload-relative 112. Root +0x2C holds 228, giving model start
P+228 = 2356. Root +0x18 holds 184, giving shader-set start P+184 = 2312.
These are file-relative reconstructions, not executed relocation.

The PWIB first segment is [16,142400). The declared top RES ends at 142394.
Its last directory entry computes [142208,142432), exceeding both ends. The
selected nested pack and blob are bounded. This does not establish whole
outer-directory containment or whole-bank validity.

Reader 0x00BC43B0 admits type 1 and variant at most 1 before calling 0x00BE2600.
Header reader 0x00C02900 copies 16 bytes and reverses its dwords at +4/+8/+12
unless the leading byte is 1. The selected leading byte is 0, the bytes at M+4
spell vfxm, and the big-endian words at M+8/M+12 are 2 and 2. The version-at-+8
comparison at 0x00BE26F4 selects the less-than-3 converter 0x00C05710 and native
setup entry 0x00C1AC80.

FileInfo table 0x010CE1D0 and ChunkInfo table 0x010D1024 join header readers
0x00C06940 and 0x00C06A10 to sizes 16 and 8. The selected legacy directory branch
at 0x00C0BB11 returns 36. Record headers therefore start at M+52. Each has a kind
byte at H+0 and a big-endian body length at H+4. Advancing by 8+bodyLength yields
one record for each kind 0 through 16 and ends exactly at M+1818. Dispatch
0x00C052F0 uses table 0x00C056C0. The selected geometry/material bodies are:

| Kind | Body | Reader |
| ---: | --- | --- |
| 10 | [3038,3362) | 0x00C041E0 |
| 11 | [3370,3908) | 0x00C051D0 |
| 12 | [3916,3988) | 0x00C04A40 |
| 14 | [4008,4050) | 0x00C048D0 |

The [seasonal model and material bindings](seasonal-model-material-bindings.md)
describe the distinct version-3-or-later indexed representation. Its library
pointers, record alignment and declaration representation are not substitutions
for this legacy path.

## Vertex and index sources

Kind 11 begins with a big-endian count of 1. Its vertex record V is [3374,3908).
Big-endian u16 fields V+2 and V+4 are 24 and 20. Byte V+6 is declaration count 4.
Four seven-byte declaration records begin at V+9, followed by big-endian u16
offsets 0, 8, 12 and 16. The extension-length byte at 3419 is 8.

Reader 0x00C051D0 stores the source offset through converted +0xD0. The body begins at M+1014. Its arithmetic at 0x00C05295..0x00C052C2 is
1014+4+10+9*4+8 = 1072, relative to M. The source is
[3428,3908), 480 bytes. At 0x00C180CF..0x00C18311, the reached vertex-helper
arguments carry word V+2 before word V+4, and the buffer extent multiplies them.
Count 24 and stride 20 are the supported interpretation of this argument contract.

The selected version-2.2 branch of 0x00C04CE0..0x00C04F62 reverses the declaration
offset halfwords and declaration dwords at V+0x0C+7*i. It then visits 24 source
records at stride 20. Storage dispatch table 0x00C05174 sends codes 3 and 7 to
0x00C04ED8, which reverses one halfword per component. Code 8 reaches 0x00C04F28
without lane conversion. Format mapper 0x00C1B980 uses table 0x00C1B9F8:

| Record offset | Component count | Storage code | Format code | Selected operation | Mapped numeric format |
| ---: | ---: | ---: | ---: | --- | ---: |
| 0 | 4 | 3 | 10 | Reverse four halfword pairs | 0 |
| 8 | 4 | 8 | 13 | Preserve the four-byte span | 1 |
| 12 | 4 | 8 | 25 | Preserve the four-byte span | 10 |
| 16 | 2 | 7 | 14 | Reverse two halfword pairs | 2 |

The conditional compact descriptor prefix built by 0x00C1A100 is count 4 followed
by these (offset, component count, storage, mapped format) tuples: (0,4,3,0),
(8,4,8,1), (12,4,8,10) and (16,2,7,2). The numeric mappings establish no attribute
usage, signedness, normalization or graphics API type. The
[WRB declaration boundary](wrb-vertex-declaration.md) has its own producer,
packing and converter evidence. No join imports its API semantics into these
tuples.

Kind 12 has count 1 and record I at [3920,3988). Its raw code is 4, big-endian
u32 I+1 is 24, u32 I+5 and u16 I+9 are zero, and byte I+0x0B is 8. The selected
body starts at M+1560. The reader branch uses header length 12+8 and stores
source offset 1560+4+12+8 = 1584 through converted +0xD4. File [3940,3988)
contains 24 big-endian u16 values 0 through 23, which the reader reverses. Primitive mapper 0x00C1DF90
maps raw input 4 to internal value 4. No API topology name follows.

As a static consequence, reversing halfword pairs at offsets 0/2/4/6/16/18 of
each 20-byte source record gives a 480-byte stream with SHA-256
39f6308c78b3ca891fcf9fcc02715eabae49480bcb9060597660fd6ec43685d0.
The corresponding 48-byte little-endian index stream has SHA-256
e88624bf274aff4f35798f4bc27027683e9c1d78f132211a3cc4ae5b3decd4e3.
These derivations are not captured uploads.

## Converted part and material framing

The kind-14 body starts with count 1. Reader 0x00C048D0..0x00C04A30 stores the
converted count at +0x70 and record pointers through +0xBC, aliased to +0x6C by
0x00C052F0. The selected record is [4012,4050), 38 bytes. Its converted dword +4
is 1. Selector words at +0x22 and +0x22+2*[record+4] are both zero. These select
the first converted vertex and index records in 0x00C17F80 under normal
conversion. They establish a converted part table, not successful native part
construction.

The kind-10 body also starts with count 1. Its material record is [3042,3362),
320 bytes, with slot count 1 at +4 and parameter count 19 at +8. Reader 0x00C041E0,
including its sizing continuation 0x00C0460C..0x00C04647, gives:

| Region | Bytes | File range |
| --- | ---: | --- |
| Fixed prefix | 21 | [3042,3063) |
| Two slot-selector arrays | 4 | [3063,3067) |
| Parameter-name u16 selectors | 38 | [3067,3105) |
| Nineteen seven-byte parameter headers | 133 | [3105,3238) |
| Variable payload | 124 | [3238,3362) |

The selected slot name selectors are zero. Kind-7 name index zero is _sampler_00.
Parameter names and payload lengths do not prove eventual live binding or
semantic interpretation. This source record is distinct from the 0x2C-byte
DrawResource override element on the
[DrawResource page](vfx-drawresource-and-shaders.md#material-element).

## Shader pointers and earlier byte counts

At 0x00BE05D0, root +0x18 supplies the shader-set pointer, which remains unchanged
across the part loop. The selected set contains vertex record [4500,7040), pixel
record [7040,7584) and two zero u16 additions at set +0x10/+0x12. Source arithmetic
adds those values and 0x20 to the record pointers. Earlier holder slot +4 calls
separately receive record size minus 0x20: 2508 and 512 bytes. Holder slot +0x24
results are retained at model-object +0x60/+0x64.

Before 0x00C13990, instructions 0x00BE06EC and 0x00BE06F8 subtract 0x20 from the
saved source pointers. Under the normal return of the native material getter at
slot +0x18, whose selected target 0x00C15DF0 returns with four-byte argument
cleanup, the pending arguments in order are:

1. Name-array pointer.
2. Retained model-object +0x60 value.
3. Vertex record/header pointer, file equivalent 4500.
4. Retained model-object +0x64 value.
5. Pixel record/header pointer, file equivalent 7040.
6. Literal 1.

The selected zero additions make the header pointers exact record starts.
0x00C13990 forwards them to 0x00C0FCD0. Its flag-1 branch copies 32 bytes from each
supplied header pointer, to object +4/+0x24. These copies corroborate the pointer
interpretation. They are not byte-count arguments or proof of successful shader
creation or binding.

## Reproduction and limits

On the identified analyzed executable, bounded ghidra disasm queries with the
VA, -n count and --json options recover the readers and mappers above. Interpret
referenced jump tables as data. Independently map each cited instruction and
table range through the original PE sections, and recompute direct relative
targets. From the identified bank, follow the selected RES entry, payload-relative
root and model fields, then walk H+8+bodyLength. Decode only the stated endian
branches and selected numeric mapping entries. Source arithmetic and halfword
operations reproduce the stream hashes without client execution.

The direct source path additionally requires nonnull supplied holder/model-base
arguments, the admitted version branch, retained object identity and normal
helper completion. Source offset 1072 passes the observed four-byte alignment
test. Alternate and staging paths are separate conditions. Complete native setup
and lower creator/interface identities remain unresolved. A helper return value
of 1 is not evidence of a nonnull underlying resource.

These findings establish no native construction success, graphics API
interpretation, effect activation, color or matrix calculation, resource lifetime,
actual binding, draw execution or rendered output. They do not join this selected
model to an actual DrawResource instance or complete its final submission path.
