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

The code compares truncated destination dimensions to texture fields
`+0x18` and `+0x1C`. It passes zero for equal sizes or one for unequal
sizes to `0x00CA4130`. The meaning of this state argument as sampler
behavior is unresolved.

At `0x008E1D22..0x008E1D30`, RenderIcon passes the texture object and
render-argument `+0x10` color pointer to `0x00CA5670`. Its paths call
`0x00CBDE10` or `0x00CBA970`. Both form corners `(x,y)`,
`(x+w,y)`, `(x,y+h)`, `(x+w,y+h)` with UVs `(0,0)`,
`(1,0)`, `(0,1)`, `(1,1)`, then emit six vertex references.
Constant `0x00F54F70` is float 1.0. This selects the full normalized
texture, without an atlas-cell or source-crop selection in this path.

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
are unresolved. Final clipping, texture availability, payload format,
decoding, filtering, blend state, sampler state and final pixels are outside
these observations. Local geometry and full UVs do not establish screenshot
or pixel equivalence.

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
