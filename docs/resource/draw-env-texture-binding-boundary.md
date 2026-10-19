# DrawEnv texture-binding boundary

VA `0x00a82020` contains a guarded, indexed handoff to VA `0x00c20410`.
The callee stores the incoming 32-bit value at offset `0x35c + index * 4`
from its entry `ECX`. Its index check is signed-less-than `0x20`.
These instructions establish the call chain, argument movement, and destination
store. They do not identify the concrete virtual source or the meaning and
ownership of its returned values.

## Input and method

| Field | Value |
|---|---|
| Retail client | FINAL FANTASY XIV 1.23b, build `2012.09.19.0001` |
| Executable | `ffxivgame.exe`, x86 PE |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | `0x00400000` |
| Caller function | VA `0x00a82020`, RVA `0x00682020` |
| Inspected caller body | VA `0x00a82020` through `0x00a823df` |
| Caller producing tool | REA `analyze_function`, Ghidra `12.1.4`, default auto-analysis |
| Callee function | VA `0x00c20410`, RVA `0x00820410` |
| Inspected callee body | VA `0x00c20410` through `0x00c2047b`, 108 bytes |
| Callee producing tool | Apple LLVM `llvm-objdump --disassemble --start-address=0x00c20410 --stop-address=0x00c2047c ffxivgame.exe` |

The caller observation uses the completed function analysis, including its
instructions and referenced strings. Addresses below are VAs. Virtual slots
are byte offsets from the loaded vtable, without semantic method names.

The callee observation uses offline disassembly of the complete selected body.
Its 28 instructions cover 108 bytes contiguously. Every encoded instruction
byte was checked against the executable through its PE section mapping.
The body is in `.text` at raw file offset `0x00820410`, with SHA-256
`0ce57a9811027375030921edf8947dbbe8dfca4d62cd37145ffea6d1cc52a1d9`.
The producing `llvm-objdump` executable has SHA-256
`bb7793802ccffe8e0613ce326f6fe8caacbd626e5318bbcf73e820a5c027aca4`.
Referenced strings were read from the mapped `.rdata` bytes. This method
establishes static instruction and data observations, not runtime behavior.

## Caller observations

At `0x00a82307`, the function pushes the string at `0x010a0018`:
`SQEX::CDev::Engine::Lay::Default::External::Dw::DrawEnv::LayoutDrawEnvObject::_InitDwDrawEnvWorkData`.
The same diagnostic call receives the source-file string at `0x0109fe84`,
`.\External\Dw\DrawEnv\LayoutDrawEnvObject.cpp`. This anchors the DrawEnv
context without resolving the virtual implementations used below.

Entry `ECX` is preserved in `EBP` at `0x00a8203e`. The function reads one explicit
stack argument at `0x00a82038` and stores it at `[EBP + 0x14]`. `ESI` is set to
`EBP + 0x04` at `0x00a8204f`. Both return paths use `RET 0x4`. These instruction
facts do not require the decompiler's inferred calling convention.

| Site | Observed operation |
|---|---|
| `0x00a82362` through `0x00a82371` | Dispatch through slot `+0x7c` with `ECX = ESI`. Save the returned `EAX` as the loop limit. Skip the handoff when it is zero. |
| `0x00a82373` through `0x00a8237e` | Dispatch through slot `+0x78` with `ECX = ESI`. Skip the handoff when returned `EAX` is zero. |
| `0x00a82380` through `0x00a82390` | Load the explicit stack argument into `ECX`. Push the preceding result and dispatch through its slot `+0x5c`. Preserve returned `EAX` in `EBX` and skip the handoff when it is zero. |
| `0x00a82392` through `0x00a8239f` | Start index `EDI` at zero. With `ECX = EBX`, push the index and dispatch through slot `+0xac`. |
| `0x00a823a1` through `0x00a823a6` | Load `ECX` from `[EBP + 0x18]`. Push returned `EAX`, then the index, and directly call `0x00c20410`. There is no intervening zero test on that returned value. |
| `0x00a823ab` through `0x00a823b2` | Increment the index and repeat while it is unsigned-less-than the saved loop limit. |

The early return sets `AL` to zero at `0x00a820b1`. The final return sets `AL`
to one at `0x00a823d0`. The interpretation of those values and the complete
source-level return type remain unresolved.

## Callee observations

At `0x00c2043a`, the function pushes the method string at `0x010d3338`:
`SQEX::CDev::Engine::Dw::Renderer::DrawEnv::SetEnvmapTexture`.
The same diagnostic call receives `.\DrawEnv.cpp` at `0x010d3374`, integer
`0xc4`, an empty string at `0x010d31c6`, and the assertion string at
`0x010d3384`, `index < NUMBER_OF_ENV_MAP_TEXTURE`.

| Site | Observed operation |
|---|---|
| `0x00c20410` through `0x00c20419` | Preserve `ESI` and `EDI`. Read the first explicit stack argument into `EDI`, compare it with `0x20`, and copy entry `ECX` into `ESI`. |
| `0x00c2041b` | Branch to `0x00c2046c` when the comparison is signed-less-than. The complete body contains no separate lower-bound check. |
| `0x00c2041d` through `0x00c20430` | On the fallthrough path, test bit zero at `0x01323910`. When clear, set that bit and write `0x00c1f550` to `0x0132390c`. |
| `0x00c2043a` through `0x00c20453` | Push the five diagnostic arguments and call indirectly through `[0x0132390c]`. |
| `0x00c20459` through `0x00c20469` | If the diagnostic call returns, load the second explicit stack argument, remove the five diagnostic arguments, store the incoming value at `[ESI + EDI * 4 + 0x35c]`, restore registers, and return with `RET 0x8`. |
| `0x00c2046c` through `0x00c20479` | On the signed-less path, load the second explicit stack argument, store it at `[ESI + EDI * 4 + 0x35c]`, restore registers, and return with `RET 0x8`. |

The second argument is at `[ESP + 0x10]` after the two register pushes and at
`[ESP + 0x24]` after the five diagnostic pushes. Both stores therefore consume
the same incoming 32-bit value. The body does not dereference that value, read
the previous destination slot, or test the incoming value for zero. The
signed-less path contains no call or explicit retain/release operation.

The body has one indirect call and no direct calls. The diagnostic callee is
uninspected. Its behavior, including whether it returns, remains unresolved.

## Interpretation and unknowns

`xivl-client-structs:data/vendor/client_function_names/candidate_symbols.json`,
row VA `0x00c20410`, records the candidate name
`SQEX::CDev::Engine::Dw::Renderer::DrawEnv::SetEnvmapTexture`. Its confidence is
`candidate`. Its string and push-site evidence is distinct from its
script-derived function binding. That qualified locator and the directly
observed method string support describing this callsite as a candidate
environment-map texture handoff.

The catalog lists push site `0x00c2043b`. The Original instruction starts at
`0x00c2043a`, so the catalog address is one byte inside that push instruction.

For nonnegative indices `0..31`, the observed address expression selects dword
offsets `0x35c..0x3d8`, a computed span of `0x80` bytes. This does not establish
a complete source-level array type. The encoded signed comparison also sends
negative signed values to the direct store path. The diagnostic path contains
no early return before its store, but the uninspected diagnostic may prevent
that store by not returning. These observations do not establish a broader
valid-index or safety contract.

The callsite chain and destination store are directly observed. The concrete
implementations of slots `+0x7c`, `+0x78`, `+0x5c`, and `+0xac` remain
unresolved. Their return values and the pointer at `[EBP + 0x18]` have no
confirmed type or semantic identity from this observation. The incoming
32-bit value's representation, ownership, and lifetime remain unresolved.
The store establishes no FILE resource selection, texture default or format,
shader alias, sampler name, downstream renderer result, or live client behavior.
The reader side of the slots and the material sampler records that select slot
0 are in
[Chara engine samplers and light inputs](chara-engine-samplers-and-light-inputs.md).
