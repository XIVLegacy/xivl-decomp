# Action queue FindVins input profile

The world-owned CharaActor construction path qualifies the three virtual
interfaces used before the queue constructs FindVins. The inherited getter
alone does not identify its returned object. Constructor arguments establish
the actor's container, world and resource scene graph on this profile.
Actual retained objects and the selected resource still require their own evidence.

## Evidence identity

Addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Retained Ghidra instruction unions were compared with the original file-backed
bytes and independently decoded with Capstone 5.0.7 x86-32 and pefile 2024.8.26.
Original table cells and RTTI supplied the type identities. No target execution
supplied these observations.

The [queue handoff](action-queue-scheduler-handoff.md#context-and-embedded-dispatch)
owns the SCB input R, embedded controller E and staged adapter Dv.
The [character factory and world provider](attribute-resource-and-scene-binding.md#character-factory-and-world-provider)
owns the world/container construction and type-`0x11` factory registration.
Here A is the resulting CharaActor, T its RaptureActorContainer, W the
RaptureWorld and Z the RaptureResourceSceneGraph.

## Constructor input identity

`0x007C9110` retains its incoming container T. At `0x007C917A` it pushes
T as dispatch argument 4, then calls `0x007A02D0` with ECX=T+0x54 at
`0x007C9181`. For type `0x11`, index `0x11-0x0B=6` selects registered
factory `0x0079FAE0`. Its call at `0x0079FB34` supplies T as CharaActor
constructor `0x0065F180`'s second formal. The constructor continuation is:

| Caller | Call | Container position in callee inputs |
|---|---|---|
| CharaActor `0x0065F180` | `0x0065F1E2 -> 0x006329C0` | Third formal, after the inserted literal `0x11` |
| CDevActor `0x006329C0` | `0x00632A09 -> 0x007CEF80` | Third formal; a zero is appended to the input list |
| RaptureActor `0x007CEF80` | `0x007CEFC4 -> 0x00A60B80` | Third formal, separately from the helper-derived fourth formal |

Base Actor `0x00A60B80` loads its third formal at `0x00A60BDF` and
stores it at A+0x40 at `0x00A60BE6`. Each listed base-constructor call uses
ECX=A. CharaActor, CDevActor, RaptureActor and Actor return with
`RET 0x10`, `RET 0x14`, `RET 0x18` and `RET 0x10`, respectively.
Factory dispatch removes its four forwarded arguments at `0x007A031C`.

Consequently `u32(A+0x40)=T` on this construction path. Getter
`0x0061FF10` is `MOV EAX,[ECX+0x40]; RET`. Its inherited presence in
the CharaActor, CDevActor and RaptureActor tables does not classify an
arbitrary value supplied to a constructor.

## Container, world and graph dispatch

RaptureWorld construction supplies W to container constructor `0x007C9850`
at `0x0060CAE6`. The container stores W at T+0xA0 at `0x007C98AB`;
the world stores returned T at W+0x10 at `0x0060CB0B`. Its constructed
primary table is `0x00FE8ABC`.

World construction installs primary table `0x00FB6F44` at `0x0060C308`,
calls graph constructor `0x007CBE90` at `0x0060C3A0` and publishes its
return at W+8 at `0x0060C3D0`. The graph constructor installs primary
table `0x00FE96FC` at `0x007CBEDD`. The
[selected resource-root finding](attribute-resource-and-scene-binding.md#selected-physics-resource-root)
owns its broader graph/root construction.

With successful construction and retained fields/tables, the queue calls are:

| Queue call | Receiver | Original cell | Target and result |
|---|---|---|---|
| `0x0080F3FD`, virtual +0x18 | U=`u32(A+0x40)`=T | `0x00FE8AD4` | `0x007C8FF0`: W=`u32(T+0xA0)` |
| `0x0080F406`, virtual +8 | W | `0x00FB6F4C` | `0x007143E0`: Z=`u32(W+8)` |
| `0x0080F430`, virtual +0x6C(R) | Z | `0x00FE9768` | `0x007CB2E0`: the link walk below |

The first two targets are plain-RET getters with no stack arguments.
Original RTTI identifies T, W and Z as `Application::Scene::RaptureActorContainer`,
`Application::Scene::RaptureWorld` and `Application::Scene::RaptureResourceSceneGraph`.
The checked Complete Object Locators have zero object and construction-displacement
offsets. These are full primary receivers, without a secondary-interface adjustment.
Later mutation, release or replacement can invalidate the construction relations.

## Resource link walk

The complete body `[0x007CB2E0,0x007CB302)` receives one argument R:

1. A null R returns zero with `RET 4`.
2. If `u32(R+0x30)==0x66697274`, it returns unchanged R with `RET 4`.
3. Otherwise it replaces the incoming argument with `u32(R+0x14)` at
   `0x007CB2F9` and tail-dispatches the same Z's virtual +0x6C at `0x007CB300`.

With the same retained table and a finite readable +0x14 chain, this returns
the first pointer with that numeric tag, or zero when the chain ends. The
literal's bytes are `74 72 69 66`; no file or encounter identity is assigned
from that tag. This body has no cycle detector, depth bound, string lookup or
returned-pointer adjustment. Cyclic or invalid links do not establish a normal return.

## FindVins construction boundary

The queue requests 0x18 bytes at `0x0080F408`. Successful allocation reaches
the final virtual call and supplies its result P, together with `u32(E+4)`,
to constructor `0x008D0DE0` at `0x0080F439`. The final virtual call's
`RET 4` consumes R. The constructor consumes its two inputs with `RET 8`.
For zero allocation, the queue instead reaches `0x0080F43E` with zero,
without invoking this final virtual or the constructor.

The constructor installs `Application::Scene::Cut::Register::FindVins`
primary table `0x0105E23C` at `0x008D0E1C` and stores its first formal
at object+4 at `0x008D0E22`. The
[queue controller construction](action-queue-scheduler-handoff.md#queue-resource-and-owner)
stores `0x0060B370(u32(A+0x114))` at E+4. On the same world-owned factory
profile, A+0x114 retains W and that helper returns `u32(W+8)`=Z. Retaining
these fields therefore makes the first FindVins input the same Z used for
the final virtual call. This is not a global graph-identity claim.

The constructor initializes the vector at object+8 and selects a value from
its second formal P:

```text
link = u32(P+0x14)
selected = P
if link != 0:
    outer = u32(link+0x14)
    if outer != 0:
        selected = outer
```

P must be nonnull and readable for normal construction: `0x008D0E37`
dereferences P without a null check, and `0x008D0E3E` dereferences a
nonzero link. The preceding virtual walk's ability to return zero does not
make a null P a supported normal constructor input.

At `0x008D0E52`, the constructor passes &selected to `0x00813050` with
ECX=object+8, then returns its unchanged full receiver. For the initially
zero vector storage, that helper selects the slow path to `0x008CFC80`
at `0x008130AC`. That callee is outside this finding. The checked path
establishes the adapter's concrete inputs and requested insertion, without
proving successful vector publication, later FindVins behavior or P's semantic
identity. The queue handoff owns subsequent Dv staging and the separate V6 fallback.

## Reproduction and remaining inputs

Authenticate the PE above and compare original bytes with the selected
instruction unions. The constructor/factory bodies are `0x007A0450`,
`0x007C9110`, `0x007A02D0`, `0x0079FAE0`, `0x0065F180`,
`0x006329C0`, `0x007CEF80` and `0x00A60B80`. Check the world/container/graph
bodies `0x0060C2C0`, `0x007C9850`, `0x007CBE90`, the three getters
`0x0061FF10`, `0x007C8FF0`, `0x007143E0`, and `0x007CB2E0`,
`0x0080F3C0`, `0x008D0DE0` and `0x00813050`. Their checked retained
unions are contiguous; preserve any newly encountered unlisted bytes separately.
Read original cells and RTTI, including CharaActor table `0x00FC0D34`
cell `0x00FC0E70` and the primary-receiver offsets.

This profile requires retained actor/container/world/graph identities, readable
terminating links reaching a nonnull P, successful allocation and normal
constructor/helper returns. It does not establish a selected live actor,
authored bank, command, phase, normal/hard assignment or playback.
