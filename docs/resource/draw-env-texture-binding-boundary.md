# DrawEnv texture-binding boundary

VA `0x00a82020` contains a guarded, indexed handoff to VA `0x00c20410`.
The inspected instructions establish the call chain and argument movement.
They do not identify the concrete virtual source or the meaning of its returned
values.

## Input and method

| Field | Value |
|---|---|
| Retail client | FINAL FANTASY XIV 1.23b, build `2012.09.19.0001` |
| Executable | `ffxivgame.exe`, x86 PE |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | `0x00400000` |
| Function | VA `0x00a82020`, RVA `0x00682020` |
| Inspected body | VA `0x00a82020` through `0x00a823df` |
| Producing tool | REA `analyze_function`, Ghidra `12.1.4`, default auto-analysis |

The observation uses the completed function analysis, including its instructions
and referenced strings. Addresses below are VAs. Virtual slots are byte offsets
from the loaded vtable, without semantic method names.

## Observations

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

## Interpretation and unknowns

`xivl-client-structs:data/vendor/client_function_names/candidate_symbols.json`,
row VA `0x00c20410`, records the candidate name
`SQEX::CDev::Engine::Dw::Renderer::DrawEnv::SetEnvmapTexture`. Its confidence is
`candidate`. Its string and push-site evidence is distinct from its
script-derived function binding. That qualified locator supports describing
this callsite as a candidate environment-map texture handoff. The callee body
is not inspected here.

The callsite chain is directly observed. The concrete implementations of slots
`+0x7c`, `+0x78`, `+0x5c`, and `+0xac` remain unresolved. Their return values
and the pointer at `[EBP + 0x18]` have no confirmed type or semantic identity
from this observation. It establishes no FILE resource selection, texture
format, sampler name, renderer result, or live client behavior.
