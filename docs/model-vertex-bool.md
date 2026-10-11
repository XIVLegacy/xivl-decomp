# Model vertex Bool submission

The retail client computes a vertex Bool value at register `1` from a receiver's
pointer field `+0x114` and the pointed object's byte `+4`. A null pointer
gives zero. A nonzero pointer gives one iff that byte is nonzero. The same
pointer later selects between two draw routines. This is a conditional
producer contract. Actual model state and shader defaults remain unproved.

## Evidence identity

| Field | Value |
|---|---|
| Module | `ffxivgame.exe`, retail build `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | `0x00400000` |
| Producing tool | `ghidra-cli 0.2.2`, read-only Ghidra instruction, decompilation, memory and `PseudoDisassembler` queries |
| Address convention | All locators are VAs. RVA = VA - `0x00400000`. |
| Confidence | High for the cited field operations, values, argument tuple, diagnostic name and call ordering. The API interpretation uses the pinned Direct3D ABI. |

The executable's size and SHA-256 were recomputed. Read-only Ghidra queries
independently reported the same imported-program SHA-256 and image base.
Decompiler-inferred names and signatures do not establish the field claims.

## Conditional value producer

**Observation.** Routine VA `0x00C44C00` loads receiver pointer `+0x114`
at VA `0x00C44C01`. VAs `0x00C44C07` through `0x00C44C1A` initialize
a four-byte local value to zero. If the pointer is nonzero, the value becomes
one iff the pointed object's byte `+4` is nonzero. VAs `0x00C44C1D`
through `0x00C44C26` pass start register `1`, that value's address and
count `1` to VA `0x0041BF10`.

This computes a submission value. It does not measure any selected model
object or establish a default for an absent shader input.

## Cached forwarding and direct backend

**Observation.** VAs `0x0041BF10` through `0x0041BF25` forward the three
arguments through VA `0x00423050`, using the manager pointer rooted at
VA `0x0132987C`. VAs `0x00423063` through `0x0042307B` first call
comparison/cache helper VA `0x00423710`. Only a zero result reaches virtual
byte offset `0x0C`.

For this call with start register `1` and count `1`, VAs `0x00423731` through
`0x00423754` compare a four-byte cached value and copy a differing value.
VAs `0x0042375D` through `0x00423760` return zero on that copy path.
Equal cached state suppresses backend dispatch.

Constructor VA `0x00438FE0` retains its supplied device pointer at
receiver `+4` at VA `0x0043900D` and installs table VA `0x00F660F8`
at VA `0x00439015`. The table's entry at byte offset `0x0C` selects
VA `0x004386A0`. This routine loads receiver `+4`, forwards register,
data pointer and count, and invokes device virtual byte offset `0x188`
at VAs `0x004386B5` through `0x004386BB`. Its return is at
VA `0x004386BD`. Read-only `PseudoDisassembler` inspection decodes this
small routine without creating instructions or changing program state.

The retained constructor identity at VA `0x00F65CE0`, referenced at
VA `0x0043904D`, names
`SQEX::CDev::Engine::Dw::RenderInterface::D3d9::Rapture::ManagerSerialImpl::ManagerSerialImpl`.

**Interpretation.** In this direct backend, the argument shape and virtual
byte offset agree with `SetVertexShaderConstantB` in Microsoft's pinned
[IDirect3DDevice9 header](https://github.com/microsoft/win32metadata/blob/76c04c2021ef4a831a6f1e06d9566002d746139b/generation/WinSDK/RecompiledIdlHeaders/shared/d3d9.h#L429-L588).
Counting methods from `QueryInterface`, including `IUnknown`, gives
zero-based slot `98`. For PE32, `98 * 4 = 0x188`. The method receives
`StartRegister`, a `BOOL` data pointer and `BoolCount`. The pinned header
is `161463` bytes with SHA-256
`a48e8132befefce1d14a8d0ffc277e7149c3c4b45d5f7427e2ac07605d65889e`.

The observed path supplies one Bool at vertex register `1`. Cache equality
can suppress the device call. Runtime backend choice, device success and
agreement between cached and device state is unproved.

## Creation-site field anchor

**Observation.** Routine VA `0x00C44B30` tests receiver `+0x114` at
VA `0x00C44B54`. On its diagnostic path, VAs `0x00C44B7A` and
`0x00C44B8E` reference native literals at VAs `0x010D7608` and
`0x010D7658`. They identify
`SQEX::CDev::Engine::Dw::Renderer::MeshDrawTag::createGeometryInstancingMeshData`
and name the tested property `geometryInstancingMeshData_`, respectively.

The allocation path calls VA `0x00C457E0` at VA `0x00C44BBB` and writes
the result into the same receiver `+0x114` at VA `0x00C44BD9`. A
subsequent call at VA `0x00C44BDF` reaches VA `0x00C73120`.

**Interpretation.** The original diagnostic anchors that field name within
the named creator. Actual creator invocation, target model ownership,
current pointer/flag state, additional writers and the complete lifecycle
remain outside this contract.

## Draw ordering and pointer-dependent branch

**Observation.** Routine VA `0x00C439C0` retains its entry receiver in
`ESI` at VA `0x00C439DC`. The observed block requires receiver pointer
fields `+0x118` and `+0xB0` nonzero and receiver bytes `+0x131` and
`+0x132` nonzero, at VAs `0x00C439E0` through `0x00C43A0E`.

VAs `0x00C43ED4` through `0x00C43ED6` pass that same receiver to the
Bool producer VA `0x00C44C00`. VA `0x00C43EDE` then calls
VA `0x00C41AA0`. VAs `0x00C43EFD` through `0x00C43F2D` subsequently
test the same receiver field `+0x114`. Nonzero reaches VA `0x00C43670`
at callsite VA `0x00C43F0A`. Zero reaches VA `0x00C423B0` at callsite
VA `0x00C43F2D`, after a separate receiver-`+0x160` control branch.

**Interpretation.** The same pointer controls the conditional Bool producer
and the later choice between these two draw routines. Pointer presence
alone does not establish Bool true or an instanced GPU draw. Other calls
between the producer and final draw, indirect dispatch, other callers,
later overwrites and final device/shader state are outside this observation.

## Reproduction and limits

Use the identified original program. Inspect the value and argument
instructions in VA `0x00C44C00`, then the small forwarding, cache and
direct-backend routines at the cited locators. Read only the selected table
entry. Confirm its constructor install and retained identity, then compare
the device call with the pinned ABI. Follow the two creation-site literals
and their receiver-`+0x114` test and assignment. Preserve entry-receiver
provenance through the Bool call, intervening call and subsequent draw split.

The actual resource/model pointer and flag state, target creator invocation
and chosen runtime branch remain unproved. No universal `isUseInstancing`
alias or shader default follows. These facts do not establish false for all
static models, true for all instanced models or a final live Bool value.
They supply no vertex stream data, missing-input fallback, matrix payload
or retail rendering equivalence. No retail-session or device-success
validation is established.

[Vertex declaration descriptor lanes](vertex-declarations.md) records the
separate conditional declaration and stream-binding evidence.
