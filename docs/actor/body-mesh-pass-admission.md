# Scene body mesh and pass admission

The pinned body resource contains a specific mesh that matches the verified
[zero-override shader profile](body-shader-constant-join.md). Native loading
establishes actor ownership before Drawable registration, and a DrawManager
layer route connects collected tags to the same mesh method. These joins are
PROVEN within the static and synthetic scope below. Admission of an actual
scene-owned body/pass before submission remains INCOMPLETE.

The [resource input trace](body-resource-inputs.md) joins the pinned visual's
packed roots and maintained count to ModelObject slot 1 as
`list, count, 1, layer, 0, 0`. The first missing identity edge is the intended
scene actor's actual appearance/resolved root association with these bytes.
Count consistency, subsequent enable, active mesh membership, reached layer/
bucket and stable zero override still need qualification before the first
changed request. The native mesh prefix does not supply those checks.

| Examined contract | Verdict | Exact boundary |
| --- | --- | --- |
| Loaded Drawable/node/tag ownership | PROVEN for the stated constructed model types and successful loader paths | Selected resource list and active membership are separate |
| Selected body MESH to first SHD material | PROVEN for the pinned file and normal name binder | Actual loaded material slot and its full resource identity remain conditions |
| DrawManager layer/bucket to mesh dispatch | PROVEN as a conditional static route | Current scene actor, layer, bucket mode and enabled flags are not observed |
| Native prefix enforces zero override/direct-route admission | REJECTED within the finite native fragment | Nonzero override and an unread instance sentinel reach the first c0..c15 request |
| Actual body/pass and complete actor-group admission | INCOMPLETE | Actual resource identity/count, subsequent enable, active members, context stability and lifetime before submission |

## Identity and method

All native locators are VAs in retail `ffxivgame.exe` 1.23b, x86, image base
`0x00400000`. The exact executable, installed body file and selected program
identities are pinned in the shader profile's
[identity section](body-shader-constant-join.md#profile-and-identity).
This trace uses that same executable SHA-256
`9341F2B4567440B310A4D494F5CC5599CA334BA51C8042247317FF466492F2E9`.

Ghidra `DumpFunctions.java` assembly supplied search leads. Fresh pefile
2024.8.26 and Capstone 5.0.7 checks corroborated the native instructions,
tables and strings. The ModelObject loader interval
`[0x00A61F52,0x00A6234A)` was decoded directly from the PE; the flat
export does not cover that ordinary fallthrough interval. Export-only caller
searches cannot establish its absence. Selected instruction comparisons are
finite and do not establish all indirect callers or alias writes.

The [focused checker](../../tools/verify_body_mesh_admission.py) pins both
inputs, checks native locators and a bounded outer CFG, and uses Unicorn 2.1.4
for separate material-binding and pre-submission fragments. None is a complete
scene load or DrawManager execution.

## Ownership before registration

Let S be a Scene-layer CharaActor with the pinned table, M its constructed
ModelObject and D the loaded Drawable. This does not identify a local-player
S; the [scene lookup](window-actor-anchor-producer.md#target-production)
remains a separate identity input.

```text
Scene CharaActor S                         visual packed roots/count
  S+0C -> ModelObject M <----------------- Model slot 1: list, count, 1, layer, 0, 0
  S+2B5C -> visual; visual+8 = S              A61DF0
  M+D0 = S                                   C50FE0 / C510F0
    M+D8 = D <------------------------------ C4F8E0 / C502B0
    D+10C = M+D0 = S                          before C27A10 registration
      D+36C: 0xB0-byte nodes
        node+78 = D; node+7C = MeshContainer
        node+84[i] -> MeshDrawTag T
          T+118 = node; T+11C = i; T+1C = native Mesh
      D+39C..+3A0: collected tag pointers

selected native Mesh name <-------------- pinned WRB MESH ordinal 1
  C34700 -> C41440 -> T+B0 material slot
    slot+38 -> resource+44 -> definition -> selected PRAM/program
      active tag -> DrawManager layer/bucket -> C439C0
        C41990 -> C421F0 -> C44C50 -> C44F07 matrix request
```

Actor construction at `0x00A60C56..0x00A60CB2` selects a factory model or
constructs through `0x00A62CA0`, then stores the result at S+0x0C.
The common base constructor writes its actor argument to M+0xD0 at
`0x00A62D50`. It installs ModelObject table `0x0109CE2C` at
`0x00A62CD5`; RaptureModelObject constructor `0x008DD0A0` calls that base
and installs `0x010653A4` at `0x008DD0E1`. Both pinned tables have slot
1=`0x00A61DF0` and slot 82=`0x006576B0`, returning `[M+0xD8]`.
Other factory overrides require their own type join.

CharaActor table `0x00FC0D34` slot 71=`0x00CB1C80` returns `[S+0x0C]`.
Construction passes S through `0x006B7600 -> 0x006307D0 -> 0x00630D20 ->
0x00631EF0 -> 0x0061F520`; `0x0061F582` stores visual+8, and
`0x0065F86C` stores the visual at S+0x2B5C. The consumer at
`0x006BE471..0x006BE4AF` follows those owner/model/Drawable getters with
nonnull checks. These are producer-backed relationships, not a Main-layer
CharaElement plus arbitrary offsets.

In `0x00A61DF0`, the first two original stack arguments forward the resource
pointer list and count. The third argument's low byte selects the loader:
zero reaches `0x00A62014 -> 0x00C50FE0`; nonzero reaches
`0x00A61FA5 -> 0x00C510F0`. The output-address result is dereferenced,
referenced when nonnull and published at M+0xD8 at
`0x00A6203B/0x00A61FCC`, before the previous object is released.
The successful path copies M+0xD0 to D+0x10C at `0x00A62246`, before
`0x00A622A6 -> 0x00C27A10` receives D and the model's context.

Before registration, `0x00A62260/0x00A6226E -> 0x00C2DBD0` copies
initializer argument 4's low seven bits into D+0x2AC.
`0x00C27A4C -> 0x00C2DBF0` reads them as a signed seven-bit layer
index. For a valid nonnegative index below the layer count,
`0x00C27A88` selects renderer+0x18[index]; the signed comparison at
`0x00C27A68` falls back to zero when index is at least the count.
This is not a safe negative-index admission rule. On the
[body visual bridge](body-resource-inputs.md#packing-count-and-exact-model-call),
actor slot 104 supplies the layer lookup result, mapping -1 to zero. No fixed
layer for the intended body is established.

`0x00C27A93 -> 0x00C38D80` appends D to the selected layer+0x1C
vector. Separately, `0x00C27B20 -> 0x00C38BB0` uses layer+0x30.
When global byte `0x0136EB11` is zero and the native bounding-box test's
`AL & 7` is zero at `0x00C38C15/0x00C38C17`, it inserts D into
collection+0x130, whose begin/end are layer+0x164/+0x168.
Fallback query `0x00C38AD0` reads precisely that vector at
`0x00C38B16`, requires `(D[+0xA0] & mask) == mask` at
`0x00C38B6F..0x00C38B79`, and appends the complete D pointer at
`0x00C38B88`. This joins registration to the mask-1 collection below.
The nonzero-global insertion/query tree branches remain unexpanded.

`0x00C50FE0` uses resource finder `0x00C524A0`: no collected model returns
null, one reaches `0x00C4F8E0`, and multiple reach `0x00C502B0`.
`0x00C510F0` selects the latter after the same finder. A single top-level
candidate does not prove a single collected model. Both branches create
material slots through `0x00C519A0`, initialize nodes through
`0x00C341D0` at `0x00C50147/0x00C50D71`, and collect tags through
`0x00C4F6F0` at `0x00C5025F/0x00C50E0E`, before M+0xD8 publication.

`0x00C341FD/0x00C34206` retain D and its selected MeshContainer at
node+0x78/+0x7C. `0x00C33430` obtains the mesh count from that container's
+0x30..+0x34 vector, constructs 0x190-byte MeshDrawTags and publishes them
at node+0x84[i]. `0x00C42524/0x00C4252A` retain the node and ordinal.
`0x00C448EF` separately retains the native Mesh at tag+0x1C.
Collection traverses D's 0xB0-stride node array and those node child vectors.

Allocation and ownership do not establish active draw membership.
The visual path `0x006BB080` can resolve a node name through
`0x006BD412 -> 0x00C3B180` and pass zero to `0x0062E370` at
`0x006BD41F`. Its store at `0x0062E3A9` changes tag+0x1A bit 0,
which the draw/collection paths test. Separately created attachment actors
from the nine-entry controller are not root Drawable nodes; their
[ownership boundary](mesh-matrix-input.md#delivery-lifetime-and-admission)
still needs a complete group join.

## Exact body mesh and material

The selected file has a PWIB prefix, a RES at offset 16 and a nested RES at
144. Its native name table identifies the following entries and mesh:

| Item | Absolute file offset | Identity / consequence |
| --- | ---: | --- |
| First SHD, nested directory index 0 | 320 | `F03\mtc001e003tc_a.win32`; the verified profile |
| WRB, nested directory index 3 | 47348 | `F06\c001e003_top.opt16.win32`, length 58928 |
| MDL containing three MESH children | 47508 | Mesh names occur in order b, a, c |
| Selected MESH ordinal 1 | 77396 | Length 22464 |
| Selected STR header / body | 77460 / 77476 | `mtc001e003tc_a` |
| Selected RSID header / body | 77492 / 77508 | Sixteen zero identifier bytes |
| First SHD PRAM body | 13808 | Byte zero is `0x1A` |

Reader `0x00E0D120` builds the directory/name-pointer array;
`0x00E0CCD0` returns the string selected by each entry's index. The packed
child path removes the folder prefix; `0x00C79590/0x00C79310` retains the
material filename and selects its prefix before the first period.
`0x00C79689` publishes `mtc001e003tc_a` as the material node name.
Material-slot creation at `0x00C51A71 -> 0x00C3E4E0` retains its resource
at slot+0x38; `0x00C51A82 -> 0x00C3DBC0` copies its name. These are
the same [generic material producers](../resource/floor-model-render-path.md#selected-material-binding)
used by the normal loaders, with this file's exact inputs.

Native Mesh creation calls `0x00C8F520` at `0x00C8DD22` for STR and
stores the string at Mesh+0x58; `0x00C8DDB5 -> 0x00C8F990` supplies
RSID at Mesh+0x30..+0x3F. Zero RSID selects `0x00C34700`'s name branch
at `0x00C34847`. It compares the Mesh string with each material slot's
virtual name and calls `0x00C41440` on the first match at `0x00C348CE`.
The matching slot becomes tag+0xB0 at `0x00C4145C`; its constructed
definition is also stored at Mesh+0x74 at `0x00C41474`.

Thus the selected mesh can use this first SHD under normal construction and
binding. The binder does not compare program hashes or establish scene actor
identity. The replay checks a matching slot after a nonmatching slot, a missing
name and two constructed equal names selecting the first slot. The last case
demonstrates lookup semantics, not an actual retail name collision.
The first matching live slot's complete resource identity remains required.

`0x00C41479 -> 0x00C3CDD0` reads PRAM byte-zero bit 0 on this pinned
device-mode helper, whose `0x0041BAC0` returns one. `0x00C4148D` sets
tag+0x1A bit 1 accordingly. This PRAM's `0x1A` clears that bit.
Drawable direct slot 9 (`0x00C39550`) therefore skips it, and slot 10
(`0x00C39620`) dispatches it if bit 0 is enabled. Those direct partitions
are distinct from the sorted slot-11 collection below.

## Reached pass and context

`0x00C28B80` is anchored by literal `DrawManager::Draw` at `0x010D4030`.
Its first `Draw::Layer` phase selects layer pointers from renderer+0x18
and requires layer+0x184 nonzero at `0x00C29125`. This identifies the
native phase, not a semantic main-world layer or the intended actor's layer.

```text
DrawManager C28B80, enabled layer
  C2919D: context = renderer+30
  C2936F -> C59970: sorter+4 = context
  C29395/C293A3: collect mask-1 Drawable list
  C2954F -> C57FD0 -> C580E1/C57DD0
    sorter+8 = current Drawable
    Drawable slot 11 C39A20 -> clear-bit-1 tag loop -> C598B0
      tag+10 = current Drawable; require tag bit 0
      bucket mode 0, sorter+808C zero
        C59480 -> bucket+14 vector of eight-byte pairs {key, tag}
  C29556 -> C59280 -> C591D0 -> C583A0
    bucket+18/+1C begin/end -> pair's tag
    C58C43/C58C4F: tag slot 1(sorter+4 context) -> C439C0
```

The context handoff is `0x00C2936F -> 0x00C59970`, whose store is
`0x00C5998A`.
Collection at `0x00C57EB7/0x00C57EBD` dispatches Drawable slot 11,
`0x00C39A20`. Its `0x00C39ADC/0x00C39AF9` clear-bit-1 path reaches
`0x00C598B0`, which copies sorter+8 to tag+0x10 and checks bit 0.
For bucket mode zero and sorter+0x808C zero,
`0x00C59533/0x00C5953E` appends to bucket+0x14. The later consumer
reads begin/end at +0x18/+0x1C and retrieves the tag at
`0x00C58A13`. `0x00C58C43` loads the retained context and
`0x00C58C4F` dispatches tag slot 1, pinned to `0x00C439C0`.
Other bucket modes and phases require separate joins.

The [enable/list producers](../resource/floor-model-render-path.md#drawable-enable-and-render-list-admission)
establish initial D+0xA0 bit 0 and its initializer/model-slot updates.
The [body initializer argument](body-resource-inputs.md#enable-and-pre-submission-admission)
is zero on the pinned state-3 route and clears that bit before registration.
A subsequent enable transition/current flags remain conditions; actor ownership
and a successful resource bind do not imply mask-1 membership.

Renderer construction `0x00C2829C/0x00C282A4 -> 0x00C48C40`
constructs context at renderer+0x30. `0x00C4900C` initially zeros its
+0xBA4. The subsequent handoff preserves the context pointer; it does not
establish an unchanged value across layer setup and callbacks. The inspected
dispatchers do not reset +0xBA4 immediately before selection. No exhaustive
alias/copy-writer or execution-ownership proof was recovered. Zero override
at the intended first request therefore remains INCOMPLETE.

## Admission before the first request

The outer method's initial checks require nonnull tag+0x118 and +0xB0,
and nonzero enable bytes +0x131/+0x132 at
`0x00C439E0..0x00C43A0E`. Those checks do not establish the full profile.

| Ordered native site | Observation | Admission implication |
| --- | --- | --- |
| `0x00C43B1E -> 0x00C41990` | Selection reads context+0xBA4 at `0x00C419D2`; a nonzero value replaces the vertex candidate at `0x00C419DC` | Override is selected, not rejected |
| `0x00C43D6C -> 0x00C421F0` | Unconditional builder call at `0x00C421F6 -> 0x00C44C50` | Reaches first matrix construction |
| `0x00C44F07 -> 0x0041BFC0` | Requests vertex float c0..c15, count 16 | First relevant submission boundary |
| `0x00C43EC3`, `0x00C43ECF`, `0x00C43EDE` | Material, node and final binding follow the builder | Their later work cannot preflight that first request |
| `0x00C43EFF`, then `0x00C43F2D` | Reads tag+0x114 to choose direct versus instance route | That field has not been read by the examined outer prefix before the builder |

A bounded intraprocedural CFG confirms the builder dominates those later
sites, treating calls as ordinarily returning. It does not expand every callee
or rule out nested work. Tag construction zeros +0x114 at `0x00C418DC`;
constructor state alone does not qualify its current value.

Four finite native cases run the actual `0x00C439C0` prefix, selection and
override publication, `0x00C421F0`, `0x00C44C50`, normal provider, context
getters and matrix helpers. All combinations of zero/nonzero +0xBA4 and
zero/one tag+0x114 reach the actual first count-16 request. Nonzero +0xBA4
is published at `0x01374CB0` and remains unchanged during these cases.
Value one at +0x114 is deliberately an unread sentinel, not a valid instance
object. The result rejects only the claim that this prefix enforces those
profile predicates before requesting constants. It does not reject the
conditional profile or establish a faulty retail draw.

Before any correction, an admission boundary still needs the selected scene
actor's actual input/root identity and consistent count, loaded objects,
subsequent enable, current active flags and pass selection,
the exact first matching material and default/driver associations, valid
mutable indices, stable zero override, and the renderer/lifetime qualifications
from the [matrix finding](mesh-matrix-input.md). The inspected load and draw
prefixes do not provide a stable whole-group preflight/fallback boundary.
Checking a later unknown member after earlier meshes changed remains too late.

## Reproduction and limits

Run from the repository with pefile, Capstone and Unicorn 2.1.4 installed:

```powershell
python tools/verify_body_mesh_admission.py --exe <pinned-ffxivgame.exe> --asset <pinned-body-0001> --output <new-result.json>
```

The checker requires a new output file and refuses optimized Python execution
so its assertions cannot be disabled. It checks exact input identities, bounded
selected file fields, native instruction/table anchors, CFG ordering, native
binder/name/PRAM getters, direct list partitions and four prefix cases.
The binder uses synthetic runtime objects and resource associations populated
from this file's names; native resource loading is not executed. Its mesh draw
entry is a stub. The binder and prefix checks are separately invoked fragments.

The prefix's synthetic upstream selection pair replaces `0x00C3D790`.
Other fixed stubs are `0x0041BF10`, `0x0060D260`, `0x009FE250`,
`0x009FE770`, `0x00C37540`, `0x00C44F80`, `0x00C450F0`,
`0x00C47870`, `0x00C47A70` and `0x00C47BF0`; each cleanup has a
checked native RET anchor. Outer matrix sources use identity values. Execution
stops at `0x0041BFC0` before its body, cache, later material/bind work,
instance dispatch or draw. No native actor load, complete pass, queue, GPU,
client process, capture or live instrumentation is executed.

The [resource input trace](body-resource-inputs.md) owns the visual-to-Model
producer join and remaining actual appearance/request/root identity edge.
Joining that root to this loaded mesh and the later enable/reached layer record
would narrow admission further; context stability and complete-group
qualification remain separate.
