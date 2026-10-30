# Sqwt IconControl textures

An ordinary Sqwt IconControl can reach the RenderIcon texture path through its
drawing context and the WindowActor command contract. This is a static route
with update guards and a renderer-cache condition. Local command 0x14
constructs RenderIcon only when the context has no cached renderer. A cached
renderer is reused without replacement.

## Input and method

The input is retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, PE32 x86 little endian, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses below are VAs at image base `0x00400000`. Subtract that base
for RVAs.

Read-only ghidra-cli 0.2.2 decompilation, disassembly and data queries supplied
the observations. Instruction and data ranges were checked against the
original PE through its section file mapping. Original complete object
locators and type descriptors anchor the concrete RTTI identities.
Decompiler names, signatures and control-flow renderings remain
interpretations where instructions or original data do not establish them.
No client execution or texture payload inspection supplies these claims.
The sampler paths were independently checked with LLVM objdump 21.0.0.
Microsoft declarations supply API meanings, not client behavior.

## OBSERVED: ordinary control to renderer

IconControl constructor `0x0096A230` installs vtable `0x01070FCC` at
object `+0`. Its complete object locator records offset zero. Vtable slot
`+0x54` is `0x0093EF90`. Context acquisition `0x0093F880` calls that
slot and caches the result at control `+0xA0`.

The helper obtains SqwtInterface through `0x00974220` and calls interface
`+4`. RaptureSqwtInterface vtable `0x00FA2D8C` supplies
`0x005522D0`, which constructs DrawingToolContext through `0x00553D90`.
RaptureSqwtInterface constructor `0x0054E190` calls `0x009741F0` to
install the interface pointer retrieved by `0x00974220`.

The forwarding and command contracts are:

| Locator | Observation |
| --- | --- |
| `0x00969FA0..0x00969FD4` | IconControl checks the update condition, non-null context and mask `0x04` at control `+0x2A8`. It forwards the vector at `+0x2AC` through DrawingContext slot `+0x90`. |
| DrawingToolContext vtable `0x00FA28E4 + 0x90` | The entry is `0x0055AC70`. It obtains state through `0x00559DE0` and calls `0x0054F110`. That function writes a count followed by numeric IDs at state `+0x3A8` and sets mask `0x01` at `+0x5A`. |
| `0x0054EE96..0x0054EEF9` in `0x0054E890` | A count-plus-IDs vector with fewer than 5 words selects local command `0x14`. The message is resized to 9 words and copies `0x24` bytes. A single ID and a count-zero vector meet this condition. |
| `0x0054B10F..0x0054B128` in `0x0054B0D0` | The producer adds `0x15` to the local command and constructs ScenePacket through `0x004EC080`. That constructor stores actor identity at `+0x90`, command at `+0x94`, length at `+0x96` and body at `+0x10`. |
| `0x007C95D3..0x007C95FA` in `0x007C93C0` | The actor dispatcher accepts command-minus-`0x15` up to `0x100` and calls actor vtable `+0x274` with the subtracted value and body. |
| WindowActor vtable `0x00FF1CC4 + 0x274` | The entry is `0x007FA2B0`. Its default route at `0x007FA8EA..0x007FA8FF` forwards to WindowContext dispatcher `0x008CEC60`. |
| Table cell `0x008CF408` and arm `0x008CF25E..0x008CF295` | Local command `0x14` obtains a renderer through `0x008CC150` and calls renderer `+0x1C` with IDs and count. |

Factory `0x008CC150` tests context `+0xDC` at `0x008CC173`.
A non-null cache branches to reuse at `0x008CC1DD`. Only an absent cache
lets command `0x14` call `0x008DF4A0` at `0x008CC1C2`. That
constructor installs vtable `0x010668A4`, identified by original RTTI as
`Application::Scene::Actor::Window::Renderer::RenderIcon`. Its `+0x1C`
entry is list consumer `0x008DF570` and its `+4` entry is render method
`0x008E1C90`. The command does not establish an existing cache's identity.

## OBSERVED: numeric input and resource resolution

Update `0x0096A040` calls parser `0x009170A0` with control vector
`+0x2AC`, a maximum of 5 positions and default fill `-1`. The parser
uses base-zero `strtol` and comma or space separators. The constructor
initializes the vector empty. A fresh literal string `1420` therefore
produces numeric ID 1420. This does not establish a general malformed-input
or range-validation contract.

An empty string performs no token writes and has no unconditional vector
shrink. It does not prove clearing of a populated control. An actual empty
numeric vector sends count zero. When the selected consumer and renderer are
`0x008DF570` and `0x008E1C90`, the consumer stores active count at
`+0x0E` at `0x008DF5F0` and returns through `0x008DF5F7`. A zero
count causes no render-loop iteration.

WindowContext draw `0x008CE7D0` at `0x008CEA87..0x008CEAA8` passes
its world pointer through `0x0060B270`. This returns world `+0x58`.
World constructor `0x0060C2C0` at `0x0060C302..0x0060C314`
constructs RaptureTextureManager at that offset through `0x0060CF60`.
Its vtable `0x00FB6D94` supplies `0x004B6670` at `+4`.
RenderIcon calls resolver `0x00CA2170` with this manager and each
non-sentinel ID. The resolver calls manager `+4` and passes the resource
ID to `0x00CA2070`.

This joins the generic route to the canonical
[resource calculation](../actor/player-nameplate-icons.md#form-and-resource-evidence).
The manager's signed quotient `I / 10000` indexes its ten-entry base table.
The nonnegative interval `0..99999` stays within that observed table.
There is no range guard in `0x004B6670`, so this interval is a table-domain
bound, not validation of arbitrary input. ID 1420 selects resource
`0x1C59058C` by this calculation. Its payload, existence and pixels are
not established here.

RenderIcon at `0x008E1CD0..0x008E1CE9` skips exactly ID `-1` before
resolution. It does not skip zero. Zero selects `0x1C590000` by the
manager arithmetic and is not proved to mean empty, clear or missing.

Resolver `0x00CA2070..0x00CA2162` initiates a resource request on cache
miss and returns zero. It also returns zero when a cached entry's payload
getter returns zero. RenderIcon skips drawing that entry when the resolver
returns zero. No fallback-icon operation appears in the inspected RenderIcon
body. This establishes no draw while unresolved, without establishing eventual
request success or a permanent-missing policy.

## OBSERVED: local image geometry

RenderIcon at `0x008E1C90..0x008E1CBF` builds a destination rectangle
with local origin `(0, 0)`, width from render argument `+8` and height
from `+0xC`. WindowContext builds that argument through `0x008CF4A0`
from context `+0xF0`. Setter `0x008CC2D0` converts four supplied
floats to signed integers and back to floats before storing at
`+0xF0..+0xFC`. Resource dimensions do not replace the destination width
or height on the one-image path.

At `0x008E1CFD..0x008E1D1D`, the code truncates supplied dimensions
to signed integers with `CVTTSS2SI` and compares them to texture fields
`+0x18` and `+0x1C`. It passes zero for equal sizes or one for unequal
sizes to `0x00CA4130`. This comparison does not establish the transformed
screen footprint.

At `0x008E1D22..0x008E1D30`, RenderIcon passes the texture object and
render-argument `+0x10` color pointer to `0x00CA5670`. Its paths call
`0x00CBDE10` or `0x00CBA970`. Both form corners `(x,y)`,
`(x+w,y)`, `(x,y+h)`, `(x+w,y+h)` with UVs `(0,0)`,
`(1,0)`, `(0,1)`, `(1,1)`, then emit six vertex references.
Constant `0x00F54F70` is float 1.0. This selects the full normalized
texture, without an atlas-cell or source-crop selection in this path.

## OBSERVED: size-dependent numeric sampler requests

Setter `0x00CA4130` compares the argument with state `0x01376494`.
Equality returns without a flush or store. A change calls `0x00CA3A70`
before storing the new state at `0x00CA4142`. That flush calls consumer
`0x00CA3890` before its pending-geometry dispatch, so it uses the prior
state. The setter does not immediately apply the new state to the device.

At `0x00CA3973..0x00CA3A39`, state zero requests numeric arguments
`(1, 1, 2, bias)` through `0x004186E0` for samplers 0 and 1.
State one requests `(2, 2, 2, bias)`. Both read bias from `0x0137648C`.
Other size states skip these calls. Helper `0x004186E0` reaches
`0x0041CD00`, which reads signed mode M from `0x01329884` through
`0x004181C0`. Its guard at `0x0041CDB8..0x0041CE1B` yields these
requests to `0x004236A0` for each sampler:

| Size state | Signed M | Parameter 5 | Parameter 6 | Parameter 7 |
| --- | --- | --- | --- | --- |
| 0, equal | 0, 1, 2 or 3 | 1 | 1 | 2 |
| 1, unequal | 0, 1 or 2 | 2 | 3 | 2 |
| 1, unequal | 3 | 2 | 2 | 2 |
| 0 or 1 | Outside 0..3 | Skipped | Skipped | Skipped |

The table records values, not dispatch order. The route passes two as the
fourth argument to `0x0041CD00`, so its argument-zero cleanup is not
taken. These paths do not request numeric address parameters 1, 2 or 3.
This is not a general addressing-state census.

Wrapper `0x004236A0` uses manager `0x0132987C` and checks
`0x00423F50` with sampler, parameter and value. Equality suppresses
backend dispatch. A changed value updates the cache and dispatches backend
vtable `+0x20` with the same arguments. Serial method `0x00438720`
calls device slot `+0x114`. Parallel methods `0x00437040` and
`0x00433C60` retain the arguments in a command with vtable
`0x00F64928` and submit it through `0x0043C2D0`. That command's
`+4` method, `0x004352A0`, calls the supplied device's `+0x114`
slot with those arguments. Submission does not establish queue execution.

## INFERRED: Direct3D9 sampler filtering

The API identity follows the device construction path. The PE imports
`d3d9.dll!Direct3DCreate9` at IAT `0x00F3E5F8`. Thunk
`0x009FC756` reads that slot. Function `0x0041FED0` calls the thunk
with `0x20` and stores the interface at `0x01329830`.
At `0x0041E2CC..0x0041E2E6`, the client calls interface slot `+0x40`
with device output `0x01329834`. At `0x0041E3CF..0x0041E3E8`,
it passes that output to manager constructor `0x00423480`, whose backend
constructors `0x00438FE0`, `0x00438290` and `0x00434EF0`
retain the supplied device at `+4`.

Microsoft's [d3d9.h declarations](https://raw.githubusercontent.com/microsoft/win32metadata/64ed9177fb76eb244fc9051c74b8a93fdfe20873/generation/WinSDK/RecompiledIdlHeaders/shared/d3d9.h)
identify ordinal 16 as `IDirect3D9::CreateDevice`, with an
`IDirect3DDevice9` output, and device ordinal 69 as
`SetSamplerState`. Four-byte pointers place those slots at `+0x40`
and `+0x114`. This anchors the numeric requests to
[SetSamplerState](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3ddevice9-setsamplerstate).

The official [sampler parameters](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dsamplerstatetype)
identify 5, 6 and 7 as `MAGFILTER`, `MINFILTER` and `MIPFILTER`.
The official [filter values](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dtexturefiltertype)
identify 1, 2 and 3 as point, linear and anisotropic. For admitted modes,
equal sizes request point magnification and minification. Unequal sizes
request linear magnification with anisotropic minification for M=0,1,2 or
linear minification for M=3. Both request linear mip filtering. This static
request contract remains subject to state timing, cache suppression and
backend execution. It does not prove device acceptance or a pixel result.

## INFERRED: generic eligibility

The concrete context factories, command arithmetic, WindowActor dispatch and
absent-cache factory branch establish a route eligible to serve ordinary
IconControls under the observed guards. The RenderIcon RTTI namespace does
not restrict that route to nameplates. Matching producer and consumer
contracts do not establish a live queue trace or an actual cached renderer's
identity.

## UNKNOWN: presentation and other inputs

Runtime queue identity, scheduling, delivery, actual cached renderer identity
and active branch choice are unobserved. Exact generic layout measurement,
desired-size policy, accumulated screen transform and final screen placement
are unresolved. Runtime filter mode, bias, backend selection, cache contents,
queue execution, API call success, device capabilities and driver behavior
remain unknown. The native mode getter does not establish a user-facing
setting name. Final clipping, texture availability, payload format, decoding,
blend state and final pixels remain outside these observations. The sampler
requests do not establish that mip filtering affects a particular texture.
Local geometry and full UVs do not establish screenshot or pixel equivalence.

Larger lists, timer rendering, arbitrary negative IDs, out-of-table IDs,
overflow, malformed text, reload races and resource-request failure lifecycle
have no contract here. An absent IconDatas property and arbitrary dependency
property reevaluation are also uncharacterized.

## Reproduction

Use the executable identity and image base above. Inspect the listed function
entries, instruction ranges, vtable entries and original RTTI descriptors with
read-only disassembly and data queries. Compare instruction and data bytes
through the PE section mappings, and read vtable entries as 32-bit little
endian values. Decompiler output alone cannot settle the cache branch or
consumer extent.

The repository's [Ghidra query tools](../../tools/README.md#analysis-queries)
provide `DecompileToText.java` with requested VAs in `DECOMP_VAS` and
`DisassembleRanges.java` with explicit `VA:byte-count` arguments.
For the consumer, inspect `0x008DF570` through `0x008DF5F7` inclusive,
using range `0x008DF570:136`. This includes the active-count store at
`0x008DF5F0` and return. A function-only decompilation can truncate this
path because of a noreturn annotation on `_free`.

For the dispatch join, inspect the context and command locators above through
`0x008CC150` and the constructor at `0x008DF4A0`. Follow the selected
`0x010668A4` slots to `0x008DF570` and `0x008E1C90`. For resource
choice, inspect `0x0060B270`, the world constructor range,
`0x004B6670`, `0x00CA2170` and `0x00CA2070`. For geometry,
inspect `0x008CC2D0`, `0x008CF4A0`, `0x00CA5670`,
`0x00CBDE10` and `0x00CBA970`. These static queries do not execute the
client or establish request completion and live presentation.

For sampler filtering, inspect the comparison, setter, consumer, mode guard,
cache wrapper and backend methods above. Read command vtable `0x00F64928`
and decode `0x004352A0` even if the analysis listing has no function there.
Follow the import thunk, interface-output call and manager handoff before
assigning API names to slots. Compare interface declaration order with the
pinned Microsoft header and filter enums with
[d3d9types.h](https://raw.githubusercontent.com/microsoft/win32metadata/64ed9177fb76eb244fc9051c74b8a93fdfe20873/generation/WinSDK/RecompiledIdlHeaders/shared/d3d9types.h)
at the same commit. These declarations establish API meanings only.
