# Mesh matrix input and shared renderer state

The private input at `0x00C44C50` does not yet establish an isolated
presentation correction. Its source ownership, stack extent and matrix
arithmetic are supported. Intended-draw survival, every subsequent consumer's
state, renderer invalidation and complete actor admission remain separate
unresolved contracts. No installed-material overlap or unrelated draw consuming
a residual correction was established by this bounded analysis.

| Examined route or contract | Verdict | Scope or unresolved edge |
| --- | --- | --- |
| Native matrix providers and private builder | PROVEN within the stated static and synthetic scope | Producer ownership, ABI and c0..c15 source construction below |
| One outer block change covering the instance loop | REJECTED by native replacement | `0x00C4396B` rebuilds c0..c15 before each `0x00C4398E` draw |
| Changing only projected c8..c11 | REJECTED within the sampled matrix stage | Decoded world/depth consumers retain their native inputs in the finite CPU replay |
| Substitution in every eligible builder invocation | INCOMPLETE | Selected late writers/program, successor consumers, lifetime and complete admission |
| One conditional body profile's bbox/material/accepted drivers | PROVEN within pinned static analysis and finite replay | [Exact selected-state join](body-shader-constant-join.md); scene-owned mesh/pass admission remains open |
| Replacement before the named successor direct mesh draws | PROVEN within a bounded CFG | Four entries, seven direct dispatch sites, ordinary call returns |
| Replacement before every unrelated or nested draw | INCOMPLETE | Next caller/pass after the walkers and pre-builder descendant calls |
| Actor-owned terminal draw payload | INCOMPLETE | Recovered constant copies and draw tasks have no complete actor ownership contract |

These verdicts do not identify an FPS divider or an incorrect physics
operation. The evaluated boundary excludes changes to shared actor,
ModelObject, Drawable, collision, simulation, camera or packet state. A private body input would
still leave body/nameplate motion policy and runtime visual acceptance open.

## Identity and producing methods

All instruction and table addresses are VAs in retail `ffxivgame.exe` 1.23b,
build `2012.09.19.0001`, x86, image base `0x00400000`, size `15996808`, SHA-256
`9341F2B4567440B310A4D494F5CC5599CA334BA51C8042247317FF466492F2E9`.

Static observations used Ghidra's
`tools/ghidra_scripts/DumpFunctions.java` assembly exports as search leads and
were corroborated against the identified PE with pefile 2024.8.26 and
Capstone 5.0.7. Selected exported instruction spans were compared byte for byte
with the PE. The fresh fixed checks covered 39 shader/successor instruction
anchors, three corresponding vtable slots, 36 delivery/admission anchors,
three actor/model slots and four renderer/task tables. These are finite
checks, not exhaustive indirect-call searches. The delivery/admission pass
corroborated all exported instruction bytes in 39 selected assembly exports.

The [builder replay](../../tools/verify_mesh_matrix_input.py) used Unicorn
2.1.4 and NumPy 2.5.0. It executed the pinned native builder, both actual
providers and matrix helpers on synthetic affine objects. Submission at
`0x0041BFC0` and the security-cookie check were stubs. This replay did not
execute material setup, the cache, a draw, a queue or the GPU.

## Ownership and source boundary

```text
accepted scene actor/key                         complete admission unresolved
  Actor slot 71 -> ModelObject at actor+0x0C
    ModelObject slot 82 -> Drawable at model+0xD8
      Drawable slot 7: 0x00C39370
        +0x39C..+0x3A0 child vector -> selected child slot 1
          MeshDrawTag 0x00C439C0 -> 0x00C421F0 -> 0x00C44C50
            mesh+0x118 -> node; mesh+0x11C -> child index
            node+0x78 -> owning Drawable
              normal: 0x00C32BD0 -> Drawable+0x30
              node+0x98 != 0: 0x00C33C10 -> caller output matrix
            private 256-byte stack block
              0x00C44F07 -> 0x0041BFC0 -> 0x00422F90 shared float cache
                immediate device submission OR queued pool copy
                  late writers + final selected shader + ordinary draw
```

The ownership fields have producers. `0x00C341FD` stores the Drawable at
node+0x78 in the loader reached through `0x00C4F8E0`. Node child creation
`0x00C33430` publishes child pointers in node+0x84[index] and invokes
`0x00C42500`. Stores at `0x00C42524` and `0x00C4252A` establish mesh+0x118
and mesh+0x11C. `0x00C4F6F0` collects and sorts children from the Drawable's
0xB0-stride node array at +0x36C. Pinned vtables select Drawable slot 7 at
`0x00C39370` and MeshDrawTag slot 1 at `0x00C439C0`.

The builder receives the mesh in ECX and one render-context pointer at entry
ESP+4. It aligns ESP to 16 bytes, reserves 0x218 bytes, preserves ESI/EDI,
and returns with `RET 4` at `0x00C44F22`. Let Q be ESP immediately after
the alignment and before the reservation. The submitted block occupies
`[Q-0x110,Q-0x10)`, exactly 256 bytes. The replay checks this effective
address at the gateway as well as the return-stack delta.

The normal getter `0x00C32BD0` returns `[node+0x78]+0x30`.
`0x0061F0F0` copies its 64 bytes into stack storage. When node+0x98 is
nonzero, `0x00C33C10` reads the same Drawable matrix, constructs identity
basis rows and copies the entire fourth row, then writes the result to its
explicit caller output and returns that pointer. `0x00C44D25` copies this
result over the selected matrix. Thus the alternate provider selects a
translation-row matrix rather than an arbitrary external matrix provider.
Its visible calls are copy helpers; neither it nor the builder stores into
the node, Drawable or context on the inspected path.

The replay passed 32 finite affine cases with both native provider branches.
Every executed write instruction targeted the emulated stack and the
synthetic input region remained unchanged. This covers the actual alternate
provider rather than assuming purity from a stub. It does not prove exception
behavior, arbitrary pointer aliasing, actor identity or destination isolation.

## Matrix roles and register writes

Using row-major interpretations of the byte arrays, let N be the selected
matrix, A=context+0xA0, E=context+0xE0, P=context+0x160 and V=context+0x20.
Each context source spans 64 bytes. The builder submits:

| Vertex float range | Byte-array value | Sampled shader use |
| --- | --- | --- |
| c0..c3 | C = E * A * N | World-coordinate result |
| c4..c7 | transpose(inverse(C)) | c4..c6 XYZ for normal/tangent; shader defines c7 locally |
| c8..c11 | C * P | Clip-coordinate result |
| c12..c15 | C * V | Z components for view depth |

These shader uses come from decoded instructions, not CTAB names alone.
Five installed body/weapon resource samples had one distinct vertex program,
SHA-256 `DD98D6ECD1F274E91E0B36151AD06735A84F0070E84C7F819748BABFA302446A`.
D3DX9_41 disassembly and a CPU interpreter of its 18-instruction matrix stage
supported these roles for 32 synthetic finite affine inputs. The full shader,
skinning, later lighting and GPU were not executed. The named inputs were
`client/chara/pc/c001/equ/e003/top_mdl/0001`, SHA-256
`21570ADE19172245ED9BB541FA3C2C7D4A74D5CD8F19500468ABCE51F788D093`, and
`client/chara/wep/w002/equ/e038/top_mdl/0001`, SHA-256
`2282F5561BC7E75079E1F449B8ECB24B4990ADB8EE096BF89E7AA6B810424258`.
Their five bytecode spans were rehashed against these installed files.
Installed-file identity does not establish original retail asset provenance,
the accepted actor's outfit or its final selected program.

For a row-vector world translation T, an input N'=N*T gives C'=C*T.
All derived roles must follow C': the inverse transpose, C'*P and C'*V.
In the sampled affine stage, translation changes world, clip and view-depth
outputs while preserving the consumed normal/tangent XYZ result. Altering
only c8..c11 leaves the sampled world and depth outputs stale. This is a
bounded instruction-stage result, not complete shader coverage.

| Native operation | Vertex float writes or reads | Other state / boundary |
| --- | --- | --- |
| `0x00C44F07 -> 0x0041BFC0` | Writes c0..c15, count 16 | Private source feeds shared `0x00422F90` cache |
| `0x00C421F0 -> 0x00C6C160/0x00C6C0F0` | c20 and palette ranges starting at c21 | Separate bool b0 setup |
| `0x00C43340` | c169/c170, or metadata-selected one-vector bbox writes | Names `ModelBBoxScale` / `ModelBBoxOffSet` |
| `0x00C3EA90 -> 0x0041A7D0` | [r,r], r from selected table | No exclusion of c0..c15 |
| Default material `0x00C3E8B0` | [r,r], r from associated table/retained handle | Requires initialized stable index/handle pairs |
| Material `0x00C56F30`, node `0x00C56ED0` | Selected [r,r] and [r,r+3] | Distinct accepted driver vectors |
| `0x00C65D26/0x00C65D4A/0x00C65D72` | Four vectors [r,r+3] each | Selected metadata supplies each r |
| `0x00C44C00 -> 0x0041BF10 -> 0x00423050` | No vertex float write | Vertex bool b1 is separate from c1 |
| Pixel driver gateways into `0x00423090` | No vertex float write | Separate pixel float bank |
| `0x00C41AA0` normal changed-shader branch | No float upload in this branch | Binds selected wrapper+4 through `0x0041C440` |
| `0x00C423B0/0x00C43670` fixed float setup | c169/c170 | Outside c0..c15 |
| `0x00C43670` instance loop | Rebuilds c0..c15 for each instance | Ordinary draw at `0x00C4398E` |

## Intended-draw survival

After the builder, `0x00C421F0` selects palette setup. Its mesh caller then
invokes `0x00C43340` at `0x00C43D73`, before material setup. The latter can
use immediate c169/c170 or named setters at `0x00C433BD/0x00C433DB`.
The named branch's destinations require the selected bbox metadata records.

The normal mesh path calls material setup at `0x00C43EC3 -> 0x00C3ECA0`.
Its named path is `0x00C3EFA8 -> 0x00C3EA90`, then
`0x00C3EC0B -> 0x0041A7D0 -> 0x00422F90`. At `0x0041A81F`, r is read
from `[table + handle*8 - 4]`, where table is selected shader wrapper+8.
The native count is one float4. Synthetic tables selecting c0 and c13
demonstrate the gateway's range behavior, but do not show an installed
material selecting either overlap.

Material `0x00C56F30` walks container+0x48..+0x4C and calls driver slot 2
at `0x00C56F75`. Node setup `0x00C336C0` reaches
`0x00C3371E -> 0x00C56ED0`, which walks +0x34..+0x38 and calls slot 2
at `0x00C56F15`. Factory-selected vertex writers include `0x00C22A90`,
`0x00C65FA0` and `0x00C662F0`, with the four-vector helper sites above.
Initialization acceptance determines actual vector membership. A factory
case alone proves neither membership nor a particular register range.

Selection follows mesh+0xB0 -> material+0x38 -> resource+0x44 ->
definition+0x2C. Selector `0x00C66F50` compares variant masks/ranks.
`0x00C419D2` reads context+0xBA4 and can override the selected vertex
container before publication at `0x00C41A6F`. Constructor zeroing does not
prove absence of an override at a draw. Final `0x00C41AA0` binding also has
cached and alternate-mode branches.

The [selected body profile](body-shader-constant-join.md) joins one installed
PRAM, native name resolution, accepted driver membership, the default
records' association and ordinary final binding. Under its explicit valid,
stable-object and zero-override conditions, bbox writes c169/c170, material
writes c177..c179 and accepted VertexLight writes c171..c176 or c179.
Those examined families leave c0..c15 untouched. A one-entry selector still
returns entry zero for unmatched masks and cannot establish pass admission.

Whole intended-draw survival remains INCOMPLETE: a scene-owned mesh/pass has
not been admitted into that profile, and other reached callees remain bounded
by the scope of their separate inspections.
The `0x00C444E0` alternate material route also reaches
`0x00C3EFD0 -> 0x00C3D5D0/0x00C3EA90` and needs its own selected inputs.

For `0x00C43670`, the nonzero `[[mesh+0x114]+4]` branch reaches
`0x00C4379F -> 0x00C71EC0` without another builder. The zero branch loops
over instance input, obtains its matrix through `0x00C3C860`, installs the
context input through `0x00C48760`, calls `0x00C44C50` at `0x00C4396B`,
then draws at `0x00C4398E`. The latter publication replaces all sixteen
outer vectors. This rejects a one-time outer-block substitution for that
branch. It does not reject substitution at each eligible builder invocation.

## Subsequent consumers and nesting

A bounded intraprocedural CFG covered `0x00C439C0`, `0x00C43FD0`,
`0x00C444E0` and `0x00D66E20`. Every entry-to-direct-draw path crossed
`0x00C421F0`, which calls the builder unconditionally. The fourth entry
contains callsite `0x00D67050`, followed by `0x00D67058 -> 0x00C423B0`.
The check decoded 1,092 instructions and seven direct dispatch sites, treating
calls as ordinarily returning. It did not expand every callee.

For these successor direct draws, the full c0..c15 request replaces the
previous block through the same cache-aware gateway. A cache hit is sufficient
only for the covered values that already equal that request. Partial named
writes establish only [r,r] or [r,r+3]; no other vector is thereby replaced.
The actual required successor values and selected shader still matter.

Node walkers `0x00C32D50`, `0x00C32DF0` and `0x00C32E30` call child slot 1.
The last two partition child+0x1A mask 0x02. `0x00C32D90` directly reaches
`0x00C444E0` for mask 0x01. These paths reach the known mesh method for the
pinned MeshDrawTag type, not every possible child/pass.

The mesh+0x17C slot-1 calls at `0x00C435AA`, `0x00C435C5` and `0x00C43F87`
were narrowed by their producer. `0x00C4154E` stores the pointer supplied to
`0x00C41510`. The inspected direct producer `0x00C3B940` obtains it through
`0x00C71970`, which creates vtables `0x010F9ADC` or `0x010F9AE8`.
Their slot-1 bodies `0x007254A0` and `0x00B8D680` return constants 1 and 2,
respectively, and contain no draw. This closes that suspected nesting edge
only for those produced types.

The next caller/pass consumer after the Drawable/node walker returns remains
unresolved. The pre-builder `0x00D67032 -> 0x00C13770 -> 0x00C10F80`
descendants are also not fully expanded. The CFG does not exclude earlier
draws inside such callees or establish every unrelated actor/pass path.
No native unrelated-draw residual-value counterexample was demonstrated.
The universal no-restore successor contract remains INCOMPLETE. Every nested
original draw must remain observable exactly once; suppressing it is not
evidence of isolation.

## Delivery, lifetime and admission

The draw route is `0x00C71EC0 -> 0x0041CB10`, then
`0x0041CB5B -> 0x004232F0` to renderer slot 26. Queued draw task table
`0x00F649B8` selects processor `0x00435D60`, which issues the indexed draw.
Its six numeric draw fields contain no actor identity.

Constant cache `0x00422F90` compares/copies values and calls the wrapped
receiver on a miss. Queued implementations `0x004337E0` and `0x00436BC0`
obtain a pool copy through `0x00417AE0`, store that pointer in task+0x0C and
append through `0x0043C2D0`. The stack pointer is not retained as the queued
payload. Immediate and queued receivers remain separate implementations.
Neither the copied source nor the task makes the shared destination
actor-owned.

| Native lifetime path | Observation | Separate limit |
| --- | --- | --- |
| `0x0041DDE0` normal completion | `0x0041DDF5 -> 0x00423380` dispatches slot 36; queued targets `0x00434650/0x00437A20` wait through `0x0043BF50`. Later `0x0041DF1A -> 0x00423390` dispatches slot 37, swaps vectors, toggles/resets the selected pool and signals consumption. | Ordinary configured wait before pool reuse |
| `0x0043C330` drain | Waits child event+0x08, executes task slot 1 in vector order and signals completion event+0x0C | Does not establish all producer/lifecycle callers |
| `0x0041DE44` early return | Occurs after slot 36, bypasses slot 37 | No new producer-vector handoff on this invocation |
| `0x0041D6A0` reset | Calls `0x00423460` at `0x0041D810`, resets cache through `0x00423E20`, then receiver slot 35. Queued `0x00434420/0x00437800` clear both child vectors through `0x0043C2E0 -> 0x00C6BE00`. Device Reset follows at `0x0041D825`. | Discards pending vector entries rather than executing their task processors; no race observed |
| Immediate owner slot 35 | `0x004384E0` returns directly | Different from queued discard |
| Child destruction `0x0043C0D0` | Sets termination through child+0xC0 at `0x0043C116`; `0x009FC830` waits on the thread handle and closes it before events/vectors are released | Thread join does not establish producer exclusion or every enclosing wakeup path |
| Queued owner destruction | `0x00434E20/0x004381C0` reach the child destructor through owner+0x0C/+0x08 | Enclosing caller admission/exclusion remains unresolved |
| Wrapped receiver replacement `0x004238D0` | Frees a different old receiver allocation and stores the replacement at `0x004238E7` | The exported direct-call search found only constructor call `0x0042367D`; no live replacement race or native generation token was established |

The immediate draw owner, vtable `0x00F660F8` slot 26=`0x00438AF0`,
constructs a 0x1C-byte stack task and calls `0x00435D60` synchronously before
return. Queued tables `0x00F65650/0x00F65B60` select
`0x00434280/0x00437660` and allocate/copy that task before appending it.
Static construction alternatives do not identify the historical receiver.

For no-restore substitution, the shorter construction/copy/original-draw
interval still requires valid source ownership, renderer/cache, pool and child
queue under one qualified delivery context. The inspected reset and teardown
sequences do not establish a single-thread caller contract, exclusion lock or
barrier that acts before the first substituted submission. They do not prove
that no such contract exists. A post-submission mismatch check, external mutex
or device state snapshot cannot supply that missing native edge. There is no
injected restoration interval in this candidate.

Scene identity must first cross object domains. Main-layer CharaElement and
Scene-layer CharaActor are different objects; a Main-layer active actor pointer
plus 0x0C is not the scene actor's ModelObject. The
[WindowActor lookup](window-actor-anchor-producer.md#target-production) passes
S+0x54 to `0x007A0150`, where S=`*(WindowActor+0x118)`. The wrapper adds
0x30 before `0x0071D420`, so the actual tree is S+0x84. Lookup returns
the matching node+0x10 pointer. The saved key equality did not retain that
resolved scene pointer/vtable.

Root ownership is supported: CharaActor's visual at +0x2B5C retains its owner
at visual+0x08. The update at `0x006BE471` follows Actor slot 71 and
ModelObject slot 82 to the Drawable. Pinned CharaActor vtable `0x00FC0D34`
slot 71=`0x00CB1C80` returns actor+0x0C; ModelObject vtable `0x010653A4`
slot 82=`0x006576B0` returns model+0xD8. This is a real ownership chain,
not a complete terminal pass set.

Ordinary equipment resource requests are a different kind of membership.
`0x00665E40` updates controller indices 0..6 from seven packed appearance
words and updates the visual. Visual routines `0x006B5E40/0x006B6690`
request face/hair, clothing and accessory banks through `0x006320C0`.
Their 0x6C-stride loading records are not scene-actor pointers or complete
Drawable/pass lists.

The attachment controller has nine 0xD0-byte entries at controller+0x50,
not merely those seven appearance updates. Constructor `0x008479A0` sets
the CharaActor owner at +0x7C4 and context at +0x7C8; initialization
`0x00847BE0` loops all nine entries. Creation `0x00846320` requests scene
type 0x12, stores the child in entry+0x08 at `0x0084637D`, and publishes
the reciprocal parent at child+0x238 at `0x00846388`. Factory type 0x12
constructs WeaponActor vtable `0x01057EE4`, whose slot 71 also selects
the actor+0x0C getter. `0x00846540` examines all nine entries and dispatches
active child slot 164. These establish admission inputs, not every child
Drawable, shader, pass or lifetime.

Complete admission remains INCOMPLETE: no recovered path enumerates the root
meshes, all populated ordinary children, additional attachments and reached
passes into one stable supported group before the first correction. Finding
an unknown member after earlier meshes were changed is too late for whole-group
fallback. The six-field terminal task cannot recover the missing group identity.
No examined payload has all actor-owned producers, consumers, lifetime and
sharing proved.

The [body mesh/pass admission trace](body-mesh-pass-admission.md) establishes
normal loaded ownership before registration, an exact installed mesh/material
match and a conditional DrawManager collection/dispatch route. The
[resource input trace](body-resource-inputs.md) joins the packed visual roots/
count to Model slot 1, with zero initial enable. Actual scene resource byte
identity, consistent count, later enable, effective active membership, layer
inputs and stable zero override remain required. The examined native
prefix reaches the first matrix request before the later material/bind work
and direct-versus-instance test; it supplies no complete profile preflight.

## Evidence that would change the remaining verdicts

| Independent obligation | Smallest next offline edge |
| --- | --- |
| Intended-draw survival | Join the captured key's actual Scene result, applied appearance and resolved root bytes to the [body profile](body-resource-identity.md), qualify count, later enable and selected state before submission, and cover other reached callees |
| Successor isolation | Expand the next actual caller/pass after the selected walker and the named pre-builder descendants; show each changed vector's required native write before its next read, including any nested original draw |
| Lifetime | Identify the enclosing native owner/thread contract or pre-submission barrier that excludes the documented reset/discard/destruction operations through source copy and original draw submission |
| Complete admission | Join root Drawable, all populated controller entries and ordinary resource-backed meshes to every reached pass before the first substituted submission |

Each edge can change its own bounded verdict. None alone proves a complete
body/nameplate correction, and no additional capture is required by this record.

## Reproduction and limits

Run the native builder replay from the repository root with a caller-supplied
copy of the identified executable:

```powershell
python tools/verify_mesh_matrix_input.py --exe '<retail-1.23b-ffxivgame.exe>'
python tools/validate_repo.py
git diff --check
```

Install `numpy`, `pefile` and `unicorn==2.1.4` in an external Python environment
for the first command. The replay reports identity, provider coverage, the
submission range, stack writes and numerical error. It does not invoke the
executable or attach to a process.

For independent static reproduction, use the committed headless runner and
`DisassembleRanges.java` with explicit bounded VA/count arguments, for example:

```powershell
tools/ghidra/run-headless.ps1 `
  -Binary '<retail-1.23b-ffxivgame.exe>' `
  -Script tools/ghidra_scripts/DisassembleRanges.java `
  -OutputDirectory tools/ghidra/logs/mesh-matrix-input `
  -ScriptArgument @('0x00C44C50:725','0x00C33C10:238',
    '0x00C421F0:41','0x00C43670:835','0x007A0150:96')
```

Use a new output directory and the Ghidra/JDK requirements in
[the tools guide](../../tools/README.md#evidence-run-contract).
Linear ranges are not recovered function boundaries or exhaustive CFGs.
For the other locators, query their containing functions or explicitly bounded
ranges by the same method. Raw assembly, imported program data and decompiled
bodies remain local analysis material.

The preserved arithmetic checks cover finite affine inputs. The named-setter
fixture manually sequences real gateway/cache code with synthetic metadata
and a mock device; its cache-hit/overlap cases do not execute the whole native
mesh method. The renderer sequencing model likewise supplies no native reset,
nesting or generation proof. No check observed values at both an actual
intended draw and every subsequent unrelated draw through their full native
paths. Those remain evidence boundaries, not claims of impossibility.
