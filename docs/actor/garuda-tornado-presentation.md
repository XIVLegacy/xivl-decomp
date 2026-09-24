# Garuda tornado presentation

This note separates the retail Garuda cast, one-shot hazard, and persistent
model-state tornado resources. Native addresses refer to the pinned FFXIV
1.23b executable with image base `0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Resource ownership

The sources establish three non-interchangeable presentation sets:

| Source | Role | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| `mon/m851/.../wss/base/0005` | Garuda tornado cast | 470,832 | `6b70fcdb5dece7deb0999210594e4ae13fd6c82f43d78400c348ad8fe4737f99` |
| `mon/m851/.../wss/base/0011` | Garuda looped tornado cast | 619,936 | `e6c5014cb0f7ff131ef40a5e0e7724f3d5c5faa392edeba4957297be415d259e` |
| `mon/m999/.../wss/base/0015` | one-shot small-tornado combat package | 429,520 | `16bcfd08dcfea38d040e7fa5faa9b939422032363908f4450bd3e1d9c8f62097` |
| `mon/m999/.../wss/base/0018` | one-shot typhoon combat package | 513,776 | `a6e52e63cd16673bf1d6633c639ca5aacdd6c88bd5739da9165f0a591d5e26fb` |
| `mon/m999/equ/e003/met_mdl/0001` | persistent tornado model states | 658,464 | `3dc782ca249deb86e914d5231829c99e48d3d2ecf6fc63c22579e1090feac501` |
| `cut/sum6g000/sum6g000` | cinematic tornado family | 10,626,512 | `c0bdcb72ac50d712642e56d29df4ef00403717f433c3fdcd553e5de0eb071e74` |

No non-generic effect payload is shared between the m851 Garuda cast banks
and the m999 hazard or persistent-state banks. A cinematic effect also does not
prove ownership by a combat command.

## Garuda cast banks

m851 WSS5 runs `cbbm_sp_b02` for 65 frames at 30 fps. Its caster scheduler
spans 1,200,000 raw units, its target scheduler spans 350,000, and the target
damage selector starts at 30,000 raw units. It references
`skl05cas01m.veffbin` and
`skl05tar01m.veffbin`, with an authored `tatumaki` token and camera shake.

m851 WSS11 runs `cbbm_sp_b04` for 90 frames at 30 fps. Its caster scheduler
spans 1,600,000 raw units and its target scheduler spans 600,000, with target
damage selection at 70,000 raw units. The bank contains two caster ActionClips
and one target ActionClip, references the m851 skill11 effect family, and carries
`tatumaki` and `LOOP`
tokens plus scene-texture, camera, draw, and filter controls. It is the
strongest Aerial Blast presentation candidate, but static assets do
not establish the exact server command selector.

## One-shot m999 hazard banks

m999 WSS15 contains `tatumaki` and `tn_hit` resources. Its caster and target
schedulers span 1,860,000 and 500,000 raw units, and its damage selector starts
at 60,000 raw units. The asset semantics support a small tornado or Great
Whirlwind
presentation.

m999 WSS18 contains `taihuu`, `tm_hit`, and `taihu_end`. Its caster and target
schedulers span 1,500,000 and 500,000 raw units, and its damage selector starts
at 70,000 raw units. The asset semantics support an arena typhoon or Eye of the
Storm
presentation.

These names are strong presentation inferences, not recovered command-to-bank
bindings. Command-result opcodes `0x0139` through `0x013c` carry command ID and
animation ID as independent fields. The animation ID already encodes the
model-dependent bank, so the client does not need a command-ID-to-WSS lookup
to display the result.

The SCB scheduler spans and selector starts above are serialized integers.
No native conversion to seconds is established for those fields; the MTB
motion durations have a separate frame and frame-rate basis.

## Persistent m999 states

Appearance ID `9114428` is the sole recovered base-model 10999 row with body
gear 3072 and resolves m999/e003. Its actor-class row has no battle class path,
so the appearance proves the model resource but not the runtime Lua class.

The e003 model exposes two supported state bits:

| Mode bit | Mask | On scheduler and effect | Off scheduler and effect | Authored role |
| ---: | ---: | --- | --- | --- |
| 4 | `0x10` | `init_msb4_1`, `tatumaki_loop/tatumaki` | `init_msb4_0`, cancels on state | small persistent tornado |
| 5 | `0x20` | `init_msb5_1`, `taihu_loop/taihu01m` | `init_msb5_0`, `taihu_end` | large persistent typhoon |

Opcode `0x0144` publishes these masks in the mode byte at payload `+0x04`.
The handler at `0x006638a4` queues type 3; the action-event drain reaches
`0x007a82f0`, which resolves `init_msb%u_1` for set bits and
`init_msb%u_0` for cleared bits. The breakage byte at `+0x00` instead queues
type 2 and cannot select these states.

The short on/off scheduler blocks are activation and cancellation controls,
not the persistent visible lifetime. The visible state remains owner-coupled
until an explicit off transition. See
[Monster model-state and color transitions](model-state-color-transitions.md)
for the shared native publication path.

## VFX controls and movement boundary

Native QIX registrations resolve serialized leaf vectors as position, angle,
and scale:

- `Position3D:CoordRoot` at `0x01301794`, update `0x00ba4c60`, defaults to
  `(0,0,0)`;
- `Angle3D:CoordRoot` at `0x01301ed4`, update `0x00ba5550`, defaults to
  `(0,0,0)` radians;
- `Scale3D:CoordRoot` at `0x01302614`, update `0x00ba5e40`, defaults to
  `(1,1,1)`.

The persistent large state is raised 0.75 client units and scaled
`(1.2,1.1,1.2)`. Audited leaf angle vectors are zero. Although the executable
implements generated angular motion as
`angle + speed*t + 0.5*acceleration*t^2`, none of the eight joined Garuda
tornado graphs instantiates that control.

Generated-position controls appear in WSS18 and the persistent small tornado,
but their native evaluator produces linear, parabolic, or damped particle
translation. `GenerateMaster` distributes particle emission along owner
movement. Neither control moves a server actor, chooses a target, sets hazard
yaw, or defines collision.

The Lentigo client classes recovered for the South-wind actor contain no
movement implementation. Actor instantiate opcode `0x00cc` supplies the class
name, while `0x00cf` and `0x00d0` provide server-authored position and speed.
The target, path, speed, retargeting, and lifetime are therefore not recoverable
from the client class or VFX graph.

## Evidence boundary

The resource graph proves presentation ownership, model-state masks, decoded
effect controls, and the distinction between particles and actor movement. It
does not recover the exact retail command selector for WSS5, WSS11, WSS15, or
WSS18, nor the South-wind movement policy. Those require original server data
or a historical retail packet capture and must remain unresolved.
