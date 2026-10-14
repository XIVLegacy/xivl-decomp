# Bounded skeleton output Float4 submission

The routine at VA `0x00c6c160` submits three consecutive Float4 blocks from
each nonnull saved output pointer, beginning at vertex constant register
`21+3*i`. A null pointer skips that table position's submission and still advances
the register start by three. This is a conditional output-block submission
contract. It does not establish a selected shader or mesh palette.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE32 image base is `0x00400000`. All locators are VA.
Subtract the image base for RVA, then map through the original section table.

The producing instrument is `llvm-objdump`, using Intel syntax and bounded
start/stop VA windows. Instructions and selected data were read directly from
the identified executable. Its size and SHA-256 matched before and after
each observation. No decompiler-inferred signature supplies a field contract.

The committed
[pointer-initialization finding](https://github.com/XIVLegacy/xivl-decomp/blob/1526a57c6ab9369b01613499895c3fef500ed6ba/docs/resource/skeleton-output-pointer-initialization.md)
and
[Joint output-access finding](https://github.com/XIVLegacy/xivl-decomp/blob/1526a57c6ab9369b01613499895c3fef500ed6ba/docs/resource/skeleton-joint-output-access.md)
own the name-lookup pointer-table population at receiver `+0x10`.
The submission observation is conditional on that established population.
It does not prove the population of every caller's receiver.

## Table position and source range

Let `R` denote the entry receiver, `ptr32` a native 32-bit pointer read,
`c` the signed dword at `R+0x0c`, and `T=ptr32(R+0x10)`.
Let `i` denote the zero-based saved-pointer-table position.
The complete body is `[0x00c6c160,0x00c6c1db)`.

At `0x00c6c19a`, the routine loads the 32-bit count into EBX.
Instructions `0x00c6c1a0..0x00c6c1a4` initialize table position `i` to zero
and skip the loop for signed `c<=0`. VA `0x00c6c1a7` initializes
the register start to `21`.

VAs `0x00c6c1b0..0x00c6c1b8` read `O_i=ptr32(T+4*i)` and branch
past submission when that pointer is zero. For a nonzero pointer,
`0x00c6c1ba..0x00c6c1c1` pass this tuple to `0x0041bfc0`:

```text
(start, data, count) = (21+3*i, O_i+0x40, 3)
```

VAs `0x00c6c1c9..0x00c6c1d1` advance `i` by one and start by three,
including the null-pointer path, then use a signed comparison against EBX
for continuation. No entries are compacted. A skipped entry supplies no
zero or identity payload. Pointer validity and count admission beyond these
instructions remain unresolved.

## Local caller order

The complete caller at `[0x00c421f0,0x00c42219)` first pushes its supplied
stack owner and calls `0x00c44c50` at `0x00c421f6`. Its entry ECX reaches
that call unchanged. Only after the preceding call returns does
`0x00c421fb` read `ptr32(owner+0x1ac)`. The nonzero path passes that
pointer as ECX to `0x00c6c160` at `0x00c42206`. The zero path instead
pushes zero and calls `0x00c6c0f0` at `0x00c42210`. Both paths return
with `RET 4`.

The complete preceding body is `[0x00c44c50,0x00c44f25)`. It preserves
its entry ECX in ESI at `0x00c44c6f` and reads `ptr32(ESI+0x118)` at
`0x00c44c71`. The stack owner is separately preserved in EDI at
`0x00c44c6c`. These observations do not establish different runtime
addresses or concrete class identities for the two inputs.

Let P denote the aligned local stack base after the preceding callee's
two register pushes. Instructions `0x00c44ee3..0x00c44f07` pass
`(start,data,count)=(0,P+0x110,16)` to the same Float4 forwarder
`0x0041bfc0` used below. The function returns with `RET 4` at
`0x00c44f22`. Its scoped body contains no read at `owner+0x1ac`.
Its outgoing helpers and the submitted blocks' semantic meaning remain
unresolved.

The observed request order is therefore the preceding sixteen-block
request at start zero, then the caller's pointer read and conditional
saved-output route. This is local returned-call order. It does not
establish a skeleton update, frame scheduling, selected shader or
successful device operation.

## MeshDrawTag table and owner-field route

The tracked
[RTTI catalog](https://github.com/XIVLegacy/xivl-decomp/blob/650640c4e9fe22c23c5cb7a8cf4b9d26ae006803/config/ffxivgame.rtti.json)
and
[slot catalog](https://github.com/XIVLegacy/xivl-decomp/blob/650640c4e9fe22c23c5cb7a8cf4b9d26ae006803/config/ffxivgame.vtable_slots.jsonl)
locate the MeshDrawTag table at VA `0x010d7a04`, with 12 slots. Original
table bytes match those targets in order. Slot 1, byte offset `+4`,
selects `0x00c439c0`. Slot 2, byte offset `+8`, selects `0x00c43fd0`.

The Original dword at `0x010d7a00` points to CompleteObjectLocator
`0x01199fbc`. Its offset is zero and its TypeDescriptor pointer is
`0x01309684`. The NUL-terminated name at `0x0130968c` is
`.?AVMeshDrawTag@Renderer@Dw@Engine@CDev@SQEX@@`. This anchors the scoped
table association without observing a runtime vtable installation.

Both scoped caller bodies retain entry ECX in ESI and their first explicit
stack argument in EDI. Let D denote that locally retained receiver and C
the supplied stack owner. Their producer and handoff sites are:

| Caller | Producer call | Owner-field store | Handoff to `0x00c421f0` |
|---|---|---|---|
| `0x00c439c0` | `0x00c43b46` | `0x00c43b4b..0x00c43b4d` transfers returned EAX through EBX into `C+0x1ac` | `0x00c43d69..0x00c43d6c` pushes EDI and supplies ESI as ECX |
| `0x00c43fd0` | `0x00c44123` | `0x00c4412a` writes returned EAX into `C+0x1ac` | `0x00c4432e..0x00c44331` pushes EDI and supplies ESI as ECX |

Both producer calls target `0x00c44f80` with ECX equal to
`ptr32(D+0x118)` and one stack argument equal to `load32(D+0x11c)`.
Here `load32(address)` denotes an untyped 32-bit value read.
The register associations are own-body observations. Outgoing callees
supply no independent interprocedural preservation or ownership contract.
The stack owner's class remains unresolved.

The complete producer body is `[0x00c44f80,0x00c45036)`. Let P denote its
entry receiver and i its explicit index argument. Its address-return route
reads `ptr32(P+0x90)` at `0x00c4500e` and calculates
`ptr32(P+0x90)+0x38*i+4` at `0x00c45014..0x00c4501f`, then returns
with `RET 4`. A flag-clear early path returns zero. Diagnostic and error
callees remain uninspected, so this is not a complete index, pointer or
failure-admission contract.

The published [pointer-initialization finding](skeleton-output-pointer-initialization.md)
owns the receiver recipe `ptr32(P+0x90)+byte_offset+4` at callsite
`0x00c3433f`, targeting `0x00c6c1f0`, with byte_offset advanced by
`0x38`. The contiguous prefix `[0x00c341d0,0x00c34349)` independently
confirms entry ECX retained in ESI, that base read and address calculation,
the direct initializer call target, and the stride increment. It omits the
remaining initializer paths and epilogue.

For a shared parent and matching ordinal, the producer address and the
published initializer receiver address agree. This is a conditional
structural match. It does not establish current initialization or pointer
population for any returned instance. The
[Joint output-access finding](skeleton-joint-output-access.md) owns the
conditional saved-pointer-table transfer, without broader block semantics.
Intervening callees could change `C+0x1ac`, including the preceding call
inside `0x00c421f0`. The later field read is not proved to retain the
producer's earlier stored value. No shader, palette, pose, lifetime,
update-timing or rendered-result contract follows from this route.

## Forwarding and direct device interpretation

The complete body `[0x0041bfc0,0x0041bfdb)` receives `(start,data,count)`
and forwards that tuple through `0x00422f90`, using the manager pointer
rooted at `0x0132987c`.

The complete body `[0x00422f90,0x00422fc3)` first invokes helper
`0x00423a40` with those arguments. Only a zero result in AL reaches
virtual slot zero of the object at `ptr32(manager+0)` at `0x00422fba`.
VA `0x00422fb1` performs that pointer read. That virtual
call has entry stack tuple `(start,count,data)`, because
`0x00422fb7..0x00422fb9` push source pointer EBX, count EDI, then start
EBP. The helper's outgoing callee supplies no additional contract here.

The constructor prefix `[0x00438fe0,0x00439070)` retains its supplied
device pointer at receiver `+4` at `0x0043900d` and installs table
`0x00f660f8` at `0x00439015`. The original table's first dword selects
`0x00438660`. Its constructor diagnostic reference at `0x0043904d`
uses the literal at `0x00f65ce0`, naming
`SQEX::CDev::Engine::Dw::RenderInterface::D3d9::Rapture::ManagerSerialImpl::ManagerSerialImpl`.
This is an incomplete constructor prefix, not a lifecycle proof.

The complete backend body `[0x00438660,0x00438680)` receives
`(start,count,data)`. It reads the retained device at receiver `+4`,
reorders the arguments to device tuple `(start,data,count)`, and calls
device virtual byte offset `0x178` at `0x00438675..0x0043867b`.

**Interpretation.** In this direct backend, the offset and argument shape
agree with `SetVertexShaderConstantF` in Microsoft's pinned
[IDirect3DDevice9 header](https://github.com/microsoft/win32metadata/blob/76c04c2021ef4a831a6f1e06d9566002d746139b/generation/WinSDK/RecompiledIdlHeaders/shared/d3d9.h#L429-L588).
Counting methods from `QueryInterface`, including `IUnknown`, gives slot
`94`. For PE32, `94*4=0x178`. The arguments are `StartRegister`, a float
data pointer and `Vector4fCount`.

The resulting source range is `O_i+[0x40,0x70)`: three Float4s in
increasing memory order, assigned register starts `21+3*i`. No transpose
appears in this submission loop. The separate four-block transpose routine
at `0x00c6c400` has no proved call or data-flow join to this route.
Helper suppression, runtime backend selection and device success remain
outside an observed submission result.

## Reproduction and limits

Verify the executable's size, SHA-256 and PE mapping. Disassemble only
the complete submission, forwarder, manager and backend bodies listed
above, the caller body, and the incomplete constructor prefix. Read the
single dword at `0x00f660f8` and the NUL-terminated diagnostic at
`0x00f65ce0`. Follow the stack offsets across all three argument orders,
then compare the device slot with the pinned header.

For the local caller order, the caller's 14 instructions cover 41 bytes.
The preceding body's 156 instructions cover 725 bytes. Both scoped bodies
were decoded contiguously and every encoded instruction byte matched the
identified executable through its PE mapping. The verified preceding body
uses two contiguous portions, `[0x00c44c50,0x00c44d53)` and
`[0x00c44d53,0x00c44f25)`, with its return included. Confirm the relative
call targets at `0x00c421f6`, `0x00c42206`, `0x00c42210` and
`0x00c44f07`, then account for the count push before the local data-pointer
calculation. The producing `llvm-objdump` executable has SHA-256
`bb7793802ccffe8e0613ce326f6fe8caacbd626e5318bbcf73e820a5c027aca4`.
The supplied stack owner's concrete class remains unresolved.


For the MeshDrawTag routes, the complete bodies
`[0x00c439c0,0x00c43fca)` and `[0x00c43fd0,0x00c444d3)` contain
389 and 321 instructions covering 1546 and 1283 bytes. The complete
producer contains 56 instructions covering 182 bytes. The incomplete
initializer prefix contains 129 instructions covering 377 bytes.
All four ranges decode contiguously and every encoded instruction byte
matched the Original PE mapping. Read only the table's preceding dword
and 12 targets, the 20-byte CompleteObjectLocator, and the TypeDescriptor
header and NUL-terminated name. Follow the producer return arithmetic,
then compare its receiver expression with the initializer callsite and
stride. These static anchors do not establish current receiver contents.

These are static executable observations. They do not establish current
receiver contents, resource selection, native execution, a live client
session or a rendered result. The associated hierarchy-output blocks remain
opaque. `jointMatrices` linkage, TEXCOORD6/7 and BLEND meanings, mesh indices,
weights, palette ordering, bind and inverse-bind meaning, axes, units,
handedness, MTB channels, animation, update timing, count and pointer safety,
cache/device agreement, native float parity and visual behavior remain
unresolved.
