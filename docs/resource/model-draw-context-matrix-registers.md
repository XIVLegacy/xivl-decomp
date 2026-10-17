# Model draw-context matrix registers

The routine at VA `0x00c41bb0` submits two 4x4 matrices from the render
context to the Float4 register forwarder. Context offset `+0x120` goes to
register `16` and context offset `+0x60` goes to register `165`, four
registers each. Register `16` is named `viewITMatrix` in the vertex shader
constant tables embedded in the executable. No embedded table names a
constant at registers `165` through `168`. The client holds no string
`projMatrix`.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE32 image base is `0x00400000`. All locators are VA. Subtract the image
base for RVA.

The instruments are `ghidra-cli 0.2.2` (Ghidra disassembly, cross-reference
and string queries on the imported executable) and a Python reader of the PE
file for the constant-table scan. Instructions and data were read from the
identified executable. Decompiler output supplied no field contract. Operand
and stack-order claims rest on the cited instructions.

The forwarder `0x0041bfc0` takes `(start, data, count)` and is described in
the [skeleton output Float4 submission page](skeleton-output-float4-submission.md),
which also owns the register `21+3*i` joint submission and the
sixteen-register block at start `0` from `0x00c44c50`.

## Forwarder bound

**Observation.** `0x0041bfc0` loads the object at VA `0x0132987c` and calls
`0x00422f90` with the same three arguments. That routine first calls the
comparison helper `0x00423a40`. The helper returns at once, treating the
request as already current, when the start register is `0x100` or more. For a
smaller start it compares `count` Float4 values against a shadow array at
`16*(start+0x14)` bytes into its receiver. At the first difference it copies
the remaining values into the shadow array and returns zero. When the helper
returns zero, `0x00422f90` calls slot `0` of the vtable of the object at
`*[0x0132987c]`, otherwise it makes no call.

**Interpretation.** The shadow array covers `256` Float4 registers, the size
of the Direct3D 9 `vs_3_0` float constant file. Two sibling forwarders at
`0x0041c040` and `0x0041c060` reach differently bounded helpers
(`0x00423b10` tests against `0x10`) and likely serve integer and Boolean
constants. The remaining sibling and slot identities are not traced here.

## Submission routine

The complete routine `0x00c41bb0` has one stack argument, the render context
C. It copies C into ESI and ECX, and its two submissions are:

| Call | Source | Start | Count | Locator |
|---|---|---|---|---|
| `0x00c41c24` | `C+0x120`, from accessor `0x00c47960` (`LEA EAX,[ECX+0x120]`) | `0x10` (16) | 4 | pushes at `0x00c41beb..0x00c41bf2` |
| `0x00c41c96` | `C+0x60`, from accessor `0x00c47940` (`LEA EAX,[ECX+0x60]`) | `0xa5` (165) | 4 | pushes at `0x00c41c5a..0x00c41c61` |

Each submission loads eight qwords from the accessor result into a stack
buffer and passes that buffer. Each buffer holds the sixteen floats of its
source in memory order. This routine does not transpose or modify either one.
It removes the three arguments with `ADD ESP,0xc` after each call and
returns with `RET 4` at `0x00c41ca2`. The accessor `0x00c47950` returns `C+0x160` and is not used by
this routine.

The register ranges are `16..19` and `165..168`.

## Callers and gate

The cross-references to `0x00c41bb0` are three calls, all inside
MeshDrawTag draw routines. The tracked RTTI anchor for MeshDrawTag
(`.?AVMeshDrawTag@Renderer@Dw@Engine@CDev@SQEX@@`, vtable `0x010d7a04`) is
recorded in the Float4 submission page.

| Call | Draw routine | Argument pushed | Gate read |
|---|---|---|---|
| `0x00c43b2e` | `0x00c439c0` | EDI, the stack argument C | `CMP byte ptr [0x0130967d]` at `0x00c43b23` |
| `0x00c4410b` | `0x00c43fd0` | EDI, the stack argument C | `0x00c44100` |
| `0x00c445de` | `0x00c444e0` | EDI, the stack argument C | `0x00c445d2` |

In each routine the receiver D (ESI) is the tag and C is the stack argument.
`0x00c41bb0` ignores D and uses only C. A zero byte at `0x0130967d` skips the
call and the following byte clear at `0x01374d88`.

**Gate value.** In `0x00c439c0` the store at `0x00c43a97` assigns
`0x0130967d` the result of comparing `dword [C+0x1a4]` against the sign
extended word at `D+4` (`0x00c43a85..0x00c43a95`). `0x00c43fd0` does the
same at `0x00c4408e..0x00c440a2`. `0x00c444e0` only reads the byte. Other
routines write it. The routine `0x00c3ff00` writes `1` to `0x0130967d` and
`0x0130967e`. It is the last call of the apply routine below. The routine
`0x00c583a0` stores a sign extended word read at offset `4` of another
object into `C+0x1a4` (stores at `0x00c5884b`, `0x00c58c66` and
`0x00c58fc2`) and other stores in it reset `C+0x1a4` to zero. The net effect
is a change test on that word. The two routines that
assign the byte overwrite a value set by `0x00c3ff00` before testing it.

## Context matrices and their writers

The render context holds 4x4 matrices at `C+0x20`, `C+0x60`, `C+0xa0` and
`C+0xe0`, and two derived matrices at `C+0x120` and `C+0x160`. Each is 64
bytes. The setters are one-instruction tail jumps to the 64-byte copier
`0x00419b60`:

| Setter | Destination |
|---|---|
| `0x00c48780` | `C+0x20` |
| `0x00c48790` | `C+0x60` |
| `0x00c48760` | `C+0xa0` |
| `0x00c48770` | `C+0xe0` |

They copy from the first stack argument. Their callers are widespread. The
set-and-apply sequences at `0x00c4d1ce`, `0x00c4d1dd` and `0x00c4d1e4`
(routine `0x00c4d070`), at `0x00c92e1d`, `0x00c92e40` and `0x00c92e47`
(routine `0x00c92880`), and at `0x00d66fef`, `0x00d66ffb` and `0x00d67002`
(routine `0x00d66e20`, repeated at `0x00d67082..0x00d67098`) each write both
matrices and then call the apply routine.

**Sources in routine `0x00c4d070`.** The routine calls `0x004186d0` with
argument `1` and `2`. `0x004186d0` returns `0x01328ff4 + 0x40*slot`, so
slots are 64-byte entries of a global array. Slot `1` is copied to a stack
buffer and handed to `0x00c48780`, giving `C+0x20`. Slot `2` is copied and
handed to `0x00c48790`, giving `C+0x60`. Other callers pass other sources.

**Save and restore.** The routine `0x00c92880` reads `C+0xa0`, `C+0xe0`,
`C+0x20` and `C+0x60` through accessors `0x00c47910`, `0x00c47920`,
`0x00c47930` and `0x00c47940` at `0x00c928de..0x00c92927`, copying each to
its stack with `0x00545670`. It later rewrites `C+0x20` and `C+0x60` from
stack copies and applies them at `0x00c92e1d..0x00c92e47`. The routine's
own string literal at `0x01107380` is `"TargetEffectCasterDrawTag::Draw"`.
Routine `0x00d66e20` also swaps the pair around a pass. Its swap is
call-level only here and its pass purpose is not traced.

### Apply routine

The routine `0x00c48ab0` takes the context in ECX and does these steps
in order:

1. Sets render-state globals through `0x0041c210`, `0x0041c2e0`,
   `0x0041d0e0`, `0x0041d0c0`, `0x0041d100` and `0x0041c270`. Their
   meaning is not traced.
2. Copies `C+0x60` into transform slot `2` and `C+0x20` into transform slot
   `1` through `0x00419020` (`0x00c48b32`, `0x00c48b47`). The helper shifts
   its slot argument left by 6 and adds `0x01328ff4`, then copies 64 bytes.
3. Writes a 4x4 identity through `0x0042fcf0` and stores it into slot `0`
   (`0x00c48b63`).
4. Computes `0x0042edb0` with ECX `C+0x60`, destination a stack buffer and
   second stack argument `C+0x20` (`0x00c48b7b..0x00c48b83`), and copies the
   result into `C+0x160` through `0x00419b60` (`0x00c48b8f`).
5. Computes `0x0042ee50` on `C+0x20` into a stack buffer, transposes it
   through `0x00a41650`, and stores the eight qwords into `C+0x120..C+0x15f`
   (`0x00c48b9b..0x00c48c16`).
6. Calls `0x00c3ff00` at `0x00c48c1e`, which sets the gate bytes, and
   clears the byte at `C+0xc54`.

**Product order of `+0x160`.** The row helper `0x0042ec50` multiplies a
four-float coefficient row (second stack argument) against the rows of its
receiver (ECX) and sums rows in order `0..3`. `0x0042edb0` calls it for each
row of its second stack argument with its own ECX as the right operand, then
writes the destination through `0x004201a0`. Its destination is the first
stack argument. At `0x00c48b7b` the pushes put `C+0x20` in the second
position and a stack buffer in the first position, and ECX is
`C+0x60`. The result is therefore the row-major product `(C+0x20) * (C+0x60)`
with `C+0x20` as left operand. The same helper identity (first argument
destination, second argument left operand, ECX right operand) is shown in
the Float4 submission page's local output matrix production.

**Derivation of `+0x120`.** The routine `0x0042ee50` reads the upper 3x3
of its receiver, forms cofactor rows, reciprocal-refines a determinant
estimate (`rcpps` followed by a Newton step against the constant `1.0`
at `0x00f54f70`), negates the receiver row `3` translation and writes a
result whose fourth column is `0, 0, 0, 1`. It assumes an affine matrix with
translation in row `3`. The stored `C+0x120` is the transpose of that
result.

## Register names from embedded constant tables

The executable embeds compiled shaders. A scan of the file for the ASCII
`CTAB` fourcc yields Direct3D shader constant tables. The header holds
size `28`, creator offset, version, constant count, constant info offset,
flags and target offset. Entries are `20` bytes: name offset, register set,
register index, register count, reserved word, type offset and default
offset. All offsets are relative to the byte after the fourcc. Register set
`2` is the Float4 set.

Three vertex-stage tables name constants at the registers of this page's
neighborhood. Table addresses are the VA of the `CTAB` fourcc:

| Table | `viewITMatrix` | `jointMatrices` | `ModelBBoxScale` | `ModelBBoxOffSet` | other |
|---|---|---|---|---|---|
| `0x010befe0` | 16, count 4 | 21, count 144 | 169 | 170 | `worldMatrix` 0, `worldITMatrix` 4 (3), `worldViewProjMatrix` 8, `worldViewMatrix` 12, `textureMatrix0` 171 (4), `depthOffSetWVPMatrix` 175 (4), `fogParam` 179, `fogColor` 180, `controlColor` 181 |
| `0x010bfb80` | 16, count 4 | 21, count 144 | 169 | 170 | `worldMatrix` 0, `worldITMatrix` 4 (3), `worldViewProjMatrix` 8, `textureMatrix0` 171 (4), `depthOffSetWVPMatrix` 175 (4), `controlColor` 179, `uvofs0` 180, `fogParam` 181, `fogColor` 182 |
| `0x010fa130` | not present | 21, count 144 | not present | not present | `isSkining` set 2 register 20, `lineLength` 169, `worldMatrix` 0, `worldViewProjMatrix` 8 |

The two string copies of `viewITMatrix` are at `0x010bf22f` and
`0x010bfe20`. `jointMatrices` ends at register `164`, since
`21 + 144 = 165`. The submission at `0x00c6c118` writes one Float4 at
register `0x14` (20), and the joint submission starts at `21+3*i`. Both match
this layout. The scan over all `105` constant tables found no Float4 constant
whose range covers registers `165` through `168`.

A separate embedded family at VA `0x00f59eb8` and `0x00f5a258` names
`$g_worldMatrix` at register `0`, `$g_viewMatrix` at `4` and
`$g_projectionMatrix` at `8`. That layout equals `4*slot` for the transform
slots above. The family belongs to other shaders. Its link to the model draw
path is not shown.

## Strings

A case-insensitive byte search of the executable for `projMatrix`, in ASCII
and in UTF-16LE, matches only inside `worldViewProjMatrix` (`0x010bf266`,
`0x010bfe57`, `0x010fa228`), the four `viewProjMatrix[n]` strings
(`0x010d24bc..0x010d24f8`) and the `shadowProjMatrix*` strings
(`0x010f8b10..0x010f8bc4`). The executable holds no standalone `projMatrix`
name, so no lookup of that name by a string it contains is possible. Retail
shader programs loaded from data files were not examined.

## Neighboring Float4 submissions

Other forwarder callers use registers adjacent to this pair. Their data
sources and meanings are not decoded here.

| Locator | Start | Count |
|---|---|---|
| `0x00c423b0` at `0x00c42479`, `0x00c4248a` | `0xa9`, `0xaa` | 1 each |
| `0x00c43340` at `0x00c43378`, `0x00c43388` | `0xa9`, `0xaa` | 1 each |
| `0x00c43670` at `0x00c43760`, `0x00c43771`, `0x00c438e0`, `0x00c438f1` | `0xa9`, `0xaa` | 1 each |
| `0x00c43590` at `0x00c435ee` | `0xac` (172) | 4 |
| `0x00c43590` at `0x00c43655` | `0xab` (171) | 1 |
| `0x00c6c0f0` at `0x00c6c118` | `0x14` (20) | 1 |

Registers `0xa9` and `0xaa` equal the `ModelBBoxScale` and `ModelBBoxOffSet`
registers in the tables above. The register `0xab..0xaf` submissions do not
match one table, so those tables are not the only layout in use.

The routine `0x00c44c50` fills registers `0..15` and reads `C+0xe0` at
`0x00c44d2c`, `C+0xa0` at `0x00c44d3c`, `C+0x160` at `0x00c44d84` and
`C+0x20` at `0x00c44dbd`, feeding `0x0042edb0` products. Register `8` is
`worldViewProjMatrix` in the tables above, which makes the `C+0x160` read
consistent with a world-view-projection product. The fields are not decoded.

## Stream flush without a buffer

**Observation.** The routine `0x0041d240` loops stream `ESI` from `0` to
`15` over sixteen records at `0x01328dcc`, stride `0x10`. For a record whose
buffer field is nonzero it pushes three record fields and the buffer's
`+0x18` field. For a record whose buffer is null it tests bit `ESI` of the
word at `[0x01329428]+8`. When the bit is set, it pushes three zeros and the
stream number and calls `0x004231e0` at `0x0041d298`. After either path it
calls `0x004231f0(stream, 1)` at `0x0041d2a6`. The word is cleared at
`0x0041d2bd`. `0x004231e0` jumps through byte offset `0x34` (slot `13`) of
the vtable of the object at `*[0x0132987c]`. The null path supplies only
zeros. It has no default color or constant payload.

**Interpretation.** The call shape suggests a stream source binding, so the
null case unbinds the stream. This page does not establish the call target.
It does not establish what the Direct3D 9 runtime supplies for a vertex
declaration element whose stream is unbound.

## Open items

| Item | State |
|---|---|
| Register `16` holds the view inverse transpose | Register number and name `viewITMatrix` are observed in two embedded tables. Value `C+0x120` is the transpose of the affine inverse of `C+0x20`. Whether `C+0x20` is the view matrix rests on the slot `1` source at `0x00c4d070` and the `$g_viewMatrix` register layout, which is an inference. |
| Product order of `C+0x160` | The row-major product `(C+0x20) * (C+0x60)` is observed. Whether it is view times projection, and which side a shader multiplies a vector on, is open. |
| Shader meaning of registers `165..168` | No embedded table covers them and no string `projMatrix` exists. Shader programs from retail data files were not examined, so a data-file name or a constant definition at those registers is open. |
| Overlap with the joint palette | The embedded palette ends at register `164`. A table with a different palette size is not excluded. |
| Default for an undeclared vertex input | The client supplies no value in the flush above. The runtime behavior is not established by the client binary. |
| Purpose of `0x00d66e20` and the render-state steps in `0x00c48ab0` | Call level only. |

## Reproduce

1. Verify the executable's size and SHA-256 above and open it in Ghidra with
   image base `0x00400000`.
2. Disassemble `0x00c41bb0` and read the pushes before the two calls to
   `0x0041bfc0` and the accessor bodies at `0x00c47940` and `0x00c47960`.
3. Query cross-references to `0x00c41bb0` and to `0x0130967d`.
4. Disassemble `0x00c48ab0`, `0x0042edb0` and `0x0042ee50`.
5. Scan the PE file for `CTAB` and parse each table with the layout above.
6. Search the executable's strings for `projMatrix`.
