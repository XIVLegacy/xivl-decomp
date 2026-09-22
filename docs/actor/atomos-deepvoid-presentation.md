# Atomos and Deepvoid presentation

This note records installed-resource and native evidence for the Atomos event
presentation. Native addresses refer to the pinned FFXIV 1.23b executable with
image base `0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Deepvoid aura ownership

The appearance value `5120` is the `head` field and selects equipment set
`e005`; it is not a body-equipment value. Eighteen recovered Deepvoid
appearance rows cover eight model families, and every associated installed
`e005/met_mdl/0001` package carries the aura lifecycle.

The package starts `awl*_on` from normal or battle idle hooks, binds its effect
to `EID_BODY_DYN`, and calls the matching `awl*_off` from death hooks. The off
scheduler explicitly cancels the on scheduler. The appearance therefore loads
the persistent red/black field without a separate event-side effect command.

| Family | Model resource | Scheduler | Terminal effect |
| --- | --- | --- | --- |
| Watcher, Pikeman, Wizard, Warrior, Slave, Scamp, Soul | m029, m030, m031, m037, m038, m505 | `awl0_on/off` | `mon_awl0` |
| Sludge | m049 | `awl1_on/off` | `mon_awl1` |
| Butcher | m054 | `awl2_on/off` | wrapper around `mon_awl0` |

The eight package hashes are:

| Model | SHA-256 |
| --- | --- |
| m029, m031, m038, m505 | `b247660782ecde4dec732b2a819028a0761a8b5114e1b5ad4c7806e86334ed76` |
| m030 | `3efa598ba83b60e2cbcb96f75a16db708a379a96cb332218e99a4fcdbbb960d2` |
| m037 | `6247a7f23e665bfe682cc4352a839811a6cbc823a700296e67a8ffc99d104e16` |
| m049 | `113b8079cbfa7198e34d682b594dfeb33f968b0000715e67ee2d5d2443fe475a` |
| m054 | `d111bf82e3477ac2a26a75e9fd524eca7c7de6d0e7b94e91945170fe0bc1fb1e` |

The on wrappers carry persistent action flags `+0x9c=0xc0` and
`+0xa0=0x101`. Their short scheduler envelope is not the visible aura
duration. The separate `cbbm_activ` and `cbbm_deact` motions are 30 frames at
30 fps and describe normal/battle transitions; their names do not prove an
event summon or despawn sequence.

## Atomos action and model-state banks

The m070 corpus consists of BID bank 0000 and WSS banks 0001 through 0010.
The BID is 591,664 bytes with SHA-256
`7c7ebfca0ec0e96737bd4420762c317c79719c327d184523c451140af9cbbf2b`.
The ten WSS banks contain 395 scheduler clips, 23 decoded motions, 24 exact
effect bindings, and 83 CIBT transitions.

| Bank | Motion | Frames / seconds | Presentation |
| --- | --- | ---: | --- |
| WSS1 | `cbbm_sp_01` | 83 / 2.767 | chest caster, middle/current, body-static target |
| WSS2 | `cbbm_sp_b01` | 90 / 3.0 | current caster and target; transition to B loop |
| WSS3 | `cbbm_sp_a01` | 90 / 3.0 | current/body-static caster, body-static target, A-to-B control |
| WSS4 | `cbbm_sp_a02` | 83 / 2.767 | chest/current caster, body-static target; transition to A loop |
| WSS5 | `cbbm_sp_02` | 90 / 3.0 | two effect actions |
| WSS6 | `cbbm_sp_03` | 90 / 3.0 | body-static caster and target |
| WSS7 | `cbbm_sp_04` | 90 / 3.0 | skill07 caster effect and reused skill01 target effect |
| WSS8 | `cbbm_sp_04` | 90 / 3.0 | motion and sound only |
| WSS9 | none | - | substatus/model-state kick |
| WSS10 | none | - | substatus/model-state kick |

WSS9 and WSS10 have byte-identical functional scheduler payloads. Their
`main` payload is 1,936 bytes with SHA-256
`7da7c8b13a2096a23de59d6719c2e2cad385cb8e6e295974273b33e7a08c0422`;
their `mon_main` payload is 944 bytes with SHA-256
`f7dc984af869e86ee956f5c3f137abf428fcd58c24ab36648acf51e72813f90c`.
The banks do not hardcode distinct state visuals.

The m070 skeleton metadata at `client/chara/mon/m070/skl/0001` is 11,392
bytes with SHA-256
`54325160e1cf992afacffa08b76f1c0d8b6582bee7e1e545ae93f5edc660b95f`.
Its `info_m070` record advertises supported model-state bit 4 (`0x10`). A
queued substate containing that bit followed by either WSS9 or WSS10 resolves
`init_msb4_1`, which launches `skill09/m070sk9c0c` and
`skill09/m070sk9c1c`. Clearing the bit resolves `init_msb4_0` and the two
corresponding skill10 effects. This is the model-state path described in
[Monster model-state and color transitions](model-state-color-transitions.md).

## A-to-B action control

WSS3 starts target scheduler `m070sk3t0` and contains one
`RaptureEffectAtoBClip`. Its factory at `0x0063a560` allocates a 0x38-byte
object constructed by `0x00821760`; the action handler is `0x00821820`.

The handler reads a signed 16-bit count at parsed record `+0x12`, followed by
signed 16-bit scheduler clip indices at `+0x16`. Each index must resolve to an
`ActionClip`, whose virtual method at `+0xcc` is called with mode 2. WSS3 has
count 1 and index 2, so it controls target ActionClip 2. It does not contain or
load another beam or tether resource.

WSS3 is therefore a strong absorption-action candidate, but WSS2 and WSS4
also contain target effects. Static assets do not identify the original
server action ID or prove which target presentation was the historical drain.

## Aetheryte variant

The b902 e001 and e002 packages share an embedded `initf_idle` scheduler,
`cbnm_id0` motion, and crystal effect `b_902vfx1`. The motion is 1,728 frames
at 30 fps, or 57.6 seconds. Their motion and VFX bytes are identical; shaders
and textures differ. e001 is the pale/cyan variant, while e002 contains orange
crystal and warm fuzz-ramp textures.

This makes b902 e002 the strongest installed persistent orange-crystal
candidate. It does not identify the original event-director selection. A
separate transient orange-white ground flare is not bound by these assets.

## Evidence boundary

The asset graph proves aura ownership, Atomos model-state behavior, action-bank
contents, and the orange material variant. Original event waves, coordinates,
action selectors, target choices, and transient flare remain unavailable
without original server logic or a historical retail capture. Authored MCB and
SCB integers remain raw units; seconds above come only from explicit MTB frame
counts and frame rates.
