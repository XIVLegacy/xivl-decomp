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
omitted blocks directly from those original bytes.

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
ModelResourceNode branch appends the resource to finder vector `+0x3C` at
`0x00C525DA`. These are ordinal comparisons, not metadata-pointer equality.

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
and nonzero bytes `+0x131/+0x132`. These include retained material and pass
state whose production is outside this geometry trace.

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
