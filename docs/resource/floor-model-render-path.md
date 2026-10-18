# Authored floor model initialization and submission

The selected floor's primary resource has a conditional native path from
`RaptureModelObject` initialization to a `DrawableObject`, mesh buffers,
`MeshDrawTag` dispatch and a D3D9 indexed-draw task. Static instructions establish
the construction and submission contracts. They do not establish that the
authored resource loaded, passed visibility tests or produced a visible frame.

## Identity and method

The executable is retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VA with PE image base `0x00400000`; RVA is VA minus that base.
Retained Ghidra assembly guided this trace. A Python PE-section reader with
pefile `2024.8.26` compared the cited listing bytes, vtables and literals with
the original executable. Capstone `5.0.7`, x86-32 mode, decoded the initializer's
omitted blocks and material-header preprocessing directly from those original
bytes. Python `struct` inspection decoded the selected RES directory, material
and component fields in both pinned model files.

The selected model resource `brt7F1C0000` is `41088` bytes with SHA-256
`1af91b0e1e5e34f676f1af2884f82eddb63062e62bee058b0260e9646dfb7d0f`.
The retained authored model has the same size and SHA-256
`c567537529a31cbd1688102477972c8449e8a10cd3df29850c1e5fac9c95e160`.
The [import contract](floor-import-contract.md) owns its preservation rules.

## Primary resource and concrete receivers

The [BGChip setup trace](bgchip-instance-resource-setup.md#primary-resource-to-model-candidate-array)
owns resource selection and the non-null primary-resource call. At
`0x00A63134..0x00A63140`, model slot 2 supplies a one-element array and invokes
slot 1. Vtable `0x010653A4` resolves these slots to `0x00A630E0` and
`0x00A61DF0`. The receiver is the complete model object in ECX. Slot 1 consumes
six stack arguments and returns with `RET 0x18` at `0x00A6247C`.

The initializer first obtains size estimates through `0x00C4EFB0` and
`0x00C4EB90`, at `0x00A61E98` and `0x00A61EFE`. Its omitted success blocks
are material: `0x00A61F52..0x00A6205F` establishes an allocation context and
calls `0x00C50FE0` or `0x00C510F0`, retaining the returned object at model
`+0xD8`. `0x00A62073..0x00A6207F` returns false when that pointer is null.
The original listing's reported `651` bytes is not the initializer's enclosing
extent and does not describe the omitted success path.

Both construction routes use `0x00C524A0`. It initializes a
`InstanceCreator::ResourceFinder` with main vtable `0x010F5734` and visits the
supplied candidates at `0x00C52532..0x00C52543`. Traversal `0x00A44BF0`
dispatches the visitor and follows resource children. `0x00C50FE0` returns
null when its collected model list is empty, uses `0x00C4F8E0` for one entry,
and uses `0x00C502B0` for multiple entries. The latter constructor remains a
separate branch; one candidate does not by itself prove one collected entry.

The selection uses native class metadata. ResourceNode metadata getter
`0x0060D750`, slot 1 of metadata vtable `0x00FB6ECC`, returns `0x01323870`.
ModelResourceNode getter `0x00C77360`, slot 1 of metadata vtable `0x010FAAD4`,
returns `0x01323818`. Initializer `0x00F06EB0` passes these two addresses to
`0x00406350`, which assigns the destination's unique ordinal at metadata
`+0x00` and stores its base-metadata pointer at `+0x04`. Thus ModelResourceNode
has ResourceNode ancestry. The metadata address and assigned ordinal are
distinct values; no fixed ordinal is assumed here.

`0x00C52B40` obtains metadata through resource `+0x0C` slot 1. It follows
metadata `+0x04` links and compares each record's first dword with the ordinal
stored at `0x01323870`. After earlier class tests, `0x00C525CE` compares the
current record's first dword with the ordinal at `0x01323818`. The matching
ModelResourceNode branch appends the resource to the complete finder's vector
object at `+0x3C` at `0x00C525DA`; its begin/end pointers are at
`+0x40/+0x44`. Main vtable `0x010F5734` slot 1 is `0x00C52B20`, whose
`0x00C52C00..0x00C52C02` forwards the same receiver to `0x00C52560`.
Append helper `0x00858140` reads vector-relative begin/end at `+0x04/+0x08`,
at `0x00858146/0x00858151`. These are ordinal comparisons, not
metadata-pointer equality.

The WRB factory supplies that concrete class. `0x00C4ABF3..0x00C4AC15`
registers ModelResourceNodeFactory vtable `0x010F2BC8` under numeric type
`0x00777262`. Its slot 3, `0x00C49E20`, calls constructor `0x00C77090` at
`0x00C49EF8`; the constructor installs main vtable `0x010FAAE4` and metadata
vtable `0x010FAAD4` at object `+0x0C`, at `0x00C770CA/0x00C770D0`.
The selected primary resource's WRB entry starts at file `0x221C`, with
`SEDBwrb\0` and version `23`. Validator `0x00C77270` recognizes that signature
and requires version at least seven, interpreting the version according to
header byte `+0x0C`. Together with the
[packed child construction](bgchip-instance-resource-setup.md#packed-resource-child-construction),
this connects WRB factory construction to the ResourceFinder's class test.
The exact loaded tree and collected list for the selected candidate remain
unobserved. The single-entry construction below is a connected conditional
branch, not an observed choice.

For the single-entry route, `0x00C4F95A/0x00C4F9A5` allocates `0x3B0` bytes
and `0x00C4F9BF` calls `0x00C3BFC0`. That constructor installs
`SQEX::CDev::Engine::Dw::Renderer::DrawableObject` main vtable `0x010D6414`
at `0x00C3C00E`, with distinct subobjects at `+0x0C`, `+0x20` and `+0x2E0`.
The model's `+0xD8` pointer is therefore not a raw serialized MESH payload.
Nonpositive drawable `+0x368` changes drawable `+0xA0` flags at
`0x00A62096..0x00A620B8`. Successful allocation alone does not imply a
nonempty drawable.

## Streams and active counts

Resource builder `0x00C75840` constructs a `MeshContainer` through
`0x00C8F160`, then calls `0x00C8DC10` for each MESH at `0x00C75B1C`.
`0x00C8F470` installs MeshContainer vtable `0x010F9568`; `0x00C72720`
installs Mesh vtable `0x010F9F8C`. The latter receiver is an allocated native
mesh, distinct from both its serialized MESH input and the DrawableObject.
`0x00C8F111` hands the mesh to the container's `+0x2C` vector through
`0x00C6D040`.

The [WRB declaration trace](wrb-vertex-declaration.md#stms-child-selection)
owns exact STMS selection and descriptor conversion. In `0x00C8DC10`, STMS
occurrence zero at `0x00C8DF38` supplies the index data. The branch at
`0x00C8DF58` requires its big-endian descriptor count to equal one.
Instructions `0x00C8DF63..0x00C8DF8D` multiply payload `+0x04` active count
by `+0x08` stride. With payload byte `+0x0E` zero, data begins at
`payload + 16 * (descriptor_count + 1)`. The nonzero-byte branch supplies a
null initial-data pointer. Call `0x00C8DFD2` creates the index-buffer wrapper
through `0x00419740` and retains it at mesh `+0xF4` at `0x00C8E000`.

Vertex STMS occurrences start at one, at `0x00C8E098..0x00C8E0A8`.
`0x00C8E0F1..0x00C8E11F` computes the same active-count times stride size
before calling vertex-buffer wrapper `0x00419640` at `0x00C8E129`.
The surrounding loop is admitted by MESH HEAD byte `+0x02 > 1` at
`0x00C8E01E`, then iterates the additional streams. Wrapper construction
checks native buffer handles and can return null; these are conditional
allocations, not observed GPU creation.

Endian helper `0x00C93770` first compares STMS payload `+0x0F` with requested
mode one. If conversion is needed, it iterates the declared descriptors and
active count, advancing by stride, swaps selected two- or four-byte components
and saves the mode byte at `0x00C938E7`. It does not substitute the padded chunk
extent for the active element count.

| Authored field | File offset | Value | Native consequence |
| --- | --- | --- | --- |
| Index STMS payload | `0x242C` | descriptors `1`, count `6`, stride `2` | `12` initial index bytes |
| Vertex STMS payload | `0x545C` | descriptors `3`, count `4`, stride `16` | `64` initial vertex bytes |
| MESH HEAD payload `+0x08` | `0x2374` | big-endian u16 `4` | mesh `+0x7A` vertex range |

The STMS chunk headers begin at file `0x241C` and `0x544C`; their retained
padded extents are `12336` and `18576`. The logical sizes are `60` and `144`.
Those extents serve chunk traversal and preservation. The buffer-size
calculations above use active counts. This supports retaining unused suffixes
without treating them as active geometry, but does not prove every native
chunk consumer accepts the authored file.

## Draw tags and indexed submission

`0x00C4FF4E..0x00C50017` obtains the component count, stores drawable
`+0x368`, and allocates `+0x36C` records with stride `0xB0`.
At `0x00C5013F`, `0x00C517E0` selects a model-resource `+0x4C` vector entry
using the component's big-endian mesh index. `0x00C341D0` receives that entry
and the DrawableObject, storing them at record `+0x7C` and `+0x78` at
`0x00C34206` and `0x00C341FD`. Its call to `0x00C33430` constructs draw tags
from the selected container's mesh list. `0x00C44830` installs
`SQEX::CDev::Engine::Dw::Renderer::MeshDrawTag` vtable `0x010D7A04`;
`0x00C448EF` stores the individual Mesh pointer at tag `+0x1C`.
`0x00C4F6F0` collects these tags into DrawableObject `+0x39C/+0x3A0`.

DrawableObject main slots 9 and 10, `0x00C39550` and `0x00C39620`, iterate
that list. Both require tag `+0x1A` bit 0 set; slot 9 additionally requires
bit 1 set, while slot 10 requires it clear. They dispatch tag slot 1 at
`0x00C395FF` or `0x00C396CF`. For MeshDrawTag, this resolves to
`0x00C439C0`. Its opening guards require non-null tag `+0x118` and `+0xB0`
and nonzero bytes `+0x131/+0x132` at `0x00C439E0..0x00C43A0E`.

## Draw-admission producers

The tag constructor writes `0x51` to tag `+0x1A` at `0x00C44883`, setting
bit 0 and clearing bit 1. Its call to `0x00C417D0` clears tag `+0xB0/+0x118`
at `0x00C4185A/0x00C41860` and initializes `+0x131/+0x132` to one at
`0x00C418FA/0x00C41900`. Mesh attachment calls that reset again at
`0x00C448F2`. In the construction loop, `0x00C33645` then calls
`0x00C42500` with the owning component record and mesh ordinal.
`0x00C42524` sets tag `+0x118` to that record, whose `+0x78/+0x7C` retain
the DrawableObject and selected MeshContainer. Thus `+0x118` is an ownership
link, rather than the material pointer.

### Selected material binding

The single-entry builder creates the drawable's material array through
`0x00C519A0` at `0x00C4FA53`, before constructing component records.
It calls binder `0x00C34700` at `0x00C5015A` when drawable `+0x360` is
nonzero. The binder walks `+0x364` slots with stride `0x54`. Nonzero mesh
`+0x30..+0x3F` selects identifier matching, also comparing mesh `+0x50`.
Otherwise `0x00C34847..0x00C348C9` compares the mesh name at `+0x58` with
each material slot's virtual name getter. A match calls `0x00C41440` at
`0x00C348CE`; exhausting the array leaves the current binding unchanged.

Both pinned models have an all-zero MESH RSID body at file
`0x23AC..0x23BB` and STR body `c0_00_ptn00` at `0x238C`. Helpers
`0x00C8F990/0x00C8F520`, called at `0x00C8DDB5/0x00C8DD22`, supply those
mesh fields. This selects name matching. RES slot zero has
`RESOURCE_TYPE` value `0x73647262` (`brds`) at file `0x9FB4` and path
`F00\c0_00_ptn00.win32` at `0xA020`. `0x00C4AD4B..0x00C4AD77` registers
MaterialResourceNodeFactory vtable `0x010F2C64` under that type. Its slot 3,
`0x00C4A060`, constructs MaterialResourceNode through `0x00C79770`.

The [packed child construction](bgchip-instance-resource-setup.md#packed-resource-child-construction)
supplies the resource name and payload. Reader `0x00E0CCD0` returns the
directory-selected string, forwarded through `0x00A3BF10` and
`0x00A44660/0x00A44730`. `0x00A44530..0x00A44548` selects the suffix
after the last folder separator. `0x00C79590` retains the filename at
material-resource `+0x54`; `0x00C79310` selects its prefix before the first
`.`. At `0x00C79689`, node slot 2 saves `c0_00_ptn00` as the node name.
Node vtable `0x010FAEFC` slots 1/2 are `0x00A3DA40/0x00A3DC20`.

Material metadata getter `0x00C79940` returns `0x01323800`.
`0x00C52583/0x00C5258B` compares its ordinal and collects matching
resources in the complete finder's vector object at `+0x14`, whose begin/end
pointers are `+0x18/+0x1C`. Instructions `0x00C519A5/0x00C519B6` read them
before creating material slots through `0x00C3E4E0`, which retains the
resource at slot `+0x38` at `0x00C3E512`.
`0x00C51A7D..0x00C51A82` copies its node name through
`0x00C3DBC0`. Material-slot vtable `0x010D646C` slot 1, `0x00C3DBD0`,
returns that name. This joins the selected resource name to the mesh-name
comparison, conditionally on resource construction and collection succeeding.

`0x00C4145C` stores the matching material slot at tag `+0xB0`.
`0x00C3DB90` follows slot `+0x38` to resource `+0x44`, the constructed
material. If that pointer is null, the binder leaves the tag flags unchanged.
Otherwise `0x00C41479..0x00C4148D` sets tag `+0x1A` bit 1 from
`0x00C3CDD0`. That helper calls `0x0041BAC0`, whose complete body returns
one, and consequently returns bit 0 of PRAM byte zero.
`0x00C414FF..0x00C41505` copies PRAM byte `+0x03` to tag `+0x133`.

The selected `SEDBshd` at file `0x90` has version `29` and endian byte zero.
Its PRAM chunk starts at `0x2070`, with body bytes `19 08 FF 00` at `0x2080`.
These material bytes are identical in the original and authored files.
Preprocessor `0x00C902A0` returns without conversion for endian byte zero;
version `29` bypasses the older-version word update at `0x00C90018`.
The [PRAM handoff](pram-packed-render-state.md#payload-handoff) retains the
body at material `+0x30`. Successful binding therefore sets tag `+0x1A`
bit 1 to one and tag `+0x133` to zero, selecting slot 9 among the two stated
dispatch branches subject to the other guards. This does not establish a
material construction or binding result in a running client.

### Mask updates and caller boundary

`0x00C32EB0` iterates component `+0x80/+0x84` tags and writes `+0x131`
at `0x00C32EF1` as `(mask & (1 << tag[+0x133])) != 0`.
`0x00C32F10` writes every tag's `+0x132` at `0x00C32F3D` as
`(mask & (1 << component[+0x28])) != 0`. These x86 shifts use the low
five selector bits. `0x00C3A200/0x00C3A310` apply the respective setters
to the DrawableObject's component records with stride `0xB0`.
`0x00C5017E..0x00C50181` copies MINS HEAD byte `+0x0A` into component
`+0x28`; `0x00C88560/0x00C88530` select that MINS and HEAD. The selected
byte at file `0x9E46` is zero in both models. Both mask tests therefore use
bit 0 on the successfully bound selected branch.

RaptureModelObject vtable `0x010653A4` slots 30/32 point to
`0x008DE260/0x008DE2D0`. They obtain model `+0xD8` through slot 82,
`0x006576B0`, and forward their mask argument to
`0x00C3A200/0x00C3A310` at `0x008DE287/0x008DE2FE` when the drawable
exists. Slot 32 also saves its mask at model `+0x294` before that guard.
These are synchronous update paths when called.

The selected BgPlateActor vtable `0x00FBA97C` has a forwarding entry at
slot 85, `0x00A5FA30`. Its complete ten-byte body loads actor `+0x0C`,
then model slot 30 at table `+0x78`, and tail-jumps with the incoming stack
argument unchanged. This closes the actor-to-model receiver and argument
forwarding edge; it does not identify a caller of that actor entry.

Actor slots 65/74 use the model's separate SoundController pointer.
Slot 74, `0x00A5FDA0`, returns zero for a null model and otherwise forwards
to model slot 85, `0x008DDA20`, which reads model `+0xF4`.
Slot 65, `0x00A5FB00`, returns zero when slot 74 is already nonzero or
the model is absent. Otherwise it invokes model slot 81, `0x00A61450`,
then the same model-slot-85 getter. Slot 81 conditionally allocates
`0x1C` bytes and calls `0x00A70910`, which installs SoundController
vtable `0x0109E6A0` and retains the model at controller `+0x04`.
The result is saved at model `+0xF4`. These calls supply no mask argument;
their model-slot-85 transfers do not identify an actor-slot-85 invocation.

The [native extra-stat route](../net/actorimpl-receiver-dispatch.md#native-extra-stat-dispatch-and-mutable-holder)
joins another generic slot-85 forwarder, `0x00575270`, to an Element's
mutable actor-implementation holder at `+0x80`. Its concrete NullActorImpl
and LuaActorImpl targets consume a packet scalar through
`ChangeActorExtraStatReceiver` or pending character state. These are
separate receiver contracts from BgPlateActor slot 85 and model slot 30.
The packet field and conditional binding order do not establish a call
to the selected floor's mask setter.

### Construction and setup ordering

Model construction `0x008DD0A0` calls base constructor `0x00A62CA0`
at `0x008DD0D4` before installing `0x010653A4` at `0x008DD0E1`.
The base constructor clears model `+0xD8` at `0x00A62D5C` and writes
`0xFFFFFFFF` to saved mask `+0x294` at `0x00A62DBA`. That saved field
is distinct from the tags created later and from slot 32's successful
application of a supplied mask to an existing drawable.

On the successful initializer path, `0x00A6203B` retains the constructed
drawable at model `+0xD8` before `0x00A62222/0x00A62234` invoke model
slot 27 with zero conditionally and then one. Slot 27 is `0x008DE860`:
it saves argument bit 0 as model `+0x29C` bit 3 and, when model bit 2 and
the drawable guards permit, updates drawable `+0xA0` bit 0. The conditional
zero call is followed by clearing model bit 2 before the one call.
These calls therefore do not unconditionally enable the drawable.
At `0x00A622B1..0x00A622C2`, the initializer passes drawable `+0xF4`
to model slot 37, `0x008DD510`, which saves it at model `+0x288` and
copies it back to an existing drawable. Neither entry is a tag-mask setter.

Slot 37 then queries the model owner through slot 6, `0x00657690`,
which reads model `+0xD0`, and queries that owner's slot 72. The actor
construction call at `0x00A60C60/0x00A60C61` passes the actor through
factory `0x008C7F70` and model constructor `0x008DD0A0` to
`0x00A62CA0`, which retains it at `+0xD0` at `0x00A62D50`.
With that constructor-retained BgPlateActor owner, slot 72 is
`0x00AB7340`, a complete zero-return body. The guard at
`0x008DD557..0x008DD559` therefore skips the downstream slot-16
context callback on this owner path.

The complete original-byte decode `0x00A61DF0..0x00A6247E`, including
the omitted success blocks, contains no direct access with displacement
`+0x294` or direct call to either mask setter. Thus this initializer body
does not directly replay the saved mask. This bounded finding does not
exclude a mask effect through an indirect call or a later update.

A concrete model-slot-32 caller, `0x008A5430`, obtains the model through
actor slot 71 and supplies `(model_slot_31_result & 0xFFFFFFF0) | argument`.
Its caller at `0x00847FC8` passes bits 22..25 of a record state word,
after testing and clearing dirty bit 26. The receiver belongs to
`CharaWeaponController`, vtable `0x0103EE4C` slot 2. Its nine records
start at controller `+0x50`, stride `0xD0`; `0x008469C0` writes the
four-bit input and sets that dirty bit. Record creation `0x00846320`
requests actor type `0x12`, factory index 7, constructing `WeaponActor`
through `0x0079FB60/0x008A5C80`. This producer supplies a different actor
type from the selected floor's type `0x15`, index 10, BgPlateActor path;
it does not establish a selected-floor mask update.

Another model-slot-32 helper, `0x0065C020`, uses model slot 31's result
as its starting mask and selects a bit with argument 1's low five bits;
it sets that bit for argument 2 zero and clears it otherwise. Its caller chain
`0x00825B97 -> 0x0065E5B0 -> 0x0065C020` passes through the
`IActor`-to-`CharaActor` cast at `0x00825B52..0x00825B65`, using type
descriptors `0x012B7C48/0x012B7810`. The selected BgPlateActor's original
RTTI hierarchy at vtable `0x00FBA97C` has no `CharaActor` base, excluding
that selected receiver from this caller's successful-cast branch.

The remaining selected static edges are the caller and mask argument of
BgPlateActor slot 85, any selected caller of model slot 32, and their order
relative to construction and draw dispatch. The initial and resource-change
[BGChip setup routes](bgchip-instance-resource-setup.md) supply resources;
the inspected setup bodies do not establish either mask invocation.
Continuing the static trace requires joining an update caller to the selected
actor/model receiver, including indirect helpers or callbacks outside the
initializer body. Actual supplied masks and active tag `+0x131/+0x132`
values remain unobserved. Constructor defaults alone also do not establish
that tag `+0x1A` bit 0 survives subsequent caller updates.

## Indexed submission

The ordinary branch calls `0x00C423B0` at `0x00C43F2D`. It selects a tag
override index buffer or mesh `+0xF4`, binds it through `0x0041D210`, and
divides buffer byte length by element width before calling `0x00C71EC0`
at `0x00C424DE`. That mesh routine passes vertex range `mesh[+0x7A]-1`
and the index count to `0x0041CB10` at `0x00C71F11`. The wrapper refuses
zero index count or absent bound index buffer, maps topology and computes
primitive count through `0x0041C540`, then dispatches six arguments through
`0x004232F0`, backend slot 26. Its range calculation is inclusive maximum
minus minimum plus one. For the ordinary triangle-list route with the authored
counts, these inputs are four vertices, six indices and two primitives.
With mesh `+0x78` zero and render-context `+0xB9D` zero, the submitted D3D9
arguments are `(4, 0, 0, 4, 0, 2)`. The table at `0x00F595C4` maps topology
four to type four, and the jump table at `0x0041C578` selects the divide-by-three
calculation at `0x0041C55C`.

The backend wrapper created by `0x00423480` is retained at global
`0x0132987C` by `0x0041E3E8`. Its selected implementation has vtable
`0x00F660F8`, `0x00F65B60` or `0x00F65650`, with slot 26 respectively
`0x00438AF0`, `0x00437660` or `0x00434280`. The first executes a task
directly; the others allocate the same `0x1C` task and enqueue it through
`0x0043C2D0`. Task vtable `0x00F649B8` slot 1 is `0x00435D60`.

At `0x00435D66..0x00435D86`, that executor calls device slot `0x148/4 = 82`
with the six stored arguments. Literal `0x00F653A0` names
`SQEX::CDev::Engine::Dw::RenderInterface::D3d9::Rapture::DrawIndexedPrimitiveTask::Process`;
literal `0x00F65378` identifies `DrawIndexedPrimitive` in its error report.
The signature matches Microsoft's
[IDirect3DDevice9::DrawIndexedPrimitive declaration](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-drawindexedprimitive):
primitive type, base vertex, minimum vertex, vertex count, start index and
primitive count. This establishes an indexed-draw submission mechanism,
including an immediate execution branch. The active backend mode, queued-task
execution and returned HRESULT for the selected floor remain unobserved.

## Remaining observation

The selected file's count relationships satisfy the recovered ordinary stream
and draw calculations. A native observation must still identify the exact
primary resource, the collected model entry, resulting DrawableObject and
MeshContainer, successful buffer handles and MeshDrawTag admission, visibility
result, selected backend and indexed draw with these counts. A successful draw
call also does not alone establish a visible floor. The
[visibility trace](floor-model-visibility.md) owns bounds and culling; the
[PRAM trace](pram-packed-render-state.md) owns packed render state. Standing and
walking require independent physics evidence.
