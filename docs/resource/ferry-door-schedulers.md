# Ferry door schedulers

The FFXIV 1.23b layout resources distinguish independently callable
voyage doors from door motion nested beneath the docked-ship scheduler. Native
addresses below refer to the pinned executable with image base `0x00400000`
and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Layout owners

| Role | Layout | Resource | Binding | Unit tree |
| --- | ---: | --- | --- | --- |
| Limsa docked ship | 196 | `sea_s0_lin01` | 456 | `sgrp_teikisen` |
| Thanalan docked ship | 496 | `wil_w0_lin01` | 456 | `sgrp_teikisen` |
| Voyage line 1 | 5142 | `srt_o0_lin01` | 323 | `sgrp_bg_door_d1` |
| Voyage line 1 | 5142 | `srt_o0_lin01` | 326 | `sgrp_bg_door_d2` |
| Voyage line 2 | 5143 | `srt_o0_lin02` | 323 | `sgrp_bg_door_d1` |
| Voyage line 2 | 5143 | `srt_o0_lin02` | 326 | `sgrp_bg_door_d2` |

The two voyage layouts contain the same four scheduler payloads. Their
instances are mirrored across the route: layout 5142 places instances 323 and
326 at `(10.2920, 7.75, -5.4863)` and `(-3.9854, 4.75, -5.4898)`; layout 5143
places them at `(-10.2920, 7.75, 5.4863)` and
`(3.9854, 4.75, 5.4898)`.

## Public aliases and effects

Each voyage-door unit tree publishes the aliases used by the actor animation
packet:

| Binding | Alias | Timeline | Controlled members |
| ---: | --- | --- | --- |
| 323 | `open` | `time_bg_door_d1_open` | four leaves, sound, two collision boxes |
| 323 | `clos` | `time_bg_door_d1_clos` | four leaves, sound, two collision boxes |
| 326 | `open` | `time_bg_door_d2_open` | two leaves, sound, two collision boxes |
| 326 | `clos` | `time_bg_door_d2_clos` | two leaves, sound, two collision boxes |

All four timelines have a raw SCB span of 450,000 units. The open timelines
disable both collision boxes at raw start value 150,000; the close timelines
enable them at raw start value 440,000. The client timebase for these SCB
fields is not established here. The payload hashes are identical in layouts
5142 and 5143:

| Timeline | SCB SHA-256 |
| --- | --- |
| `time_bg_door_d1_open` | `63d24dec761b07a91a027d467d8cec2b9aaaa2fdfd0d90c3de3fb6723ef4a9d4` |
| `time_bg_door_d1_clos` | `2dd675fd910edaa26da4cadca0fb654d9f06605e3241d2d8ca55490be20141b9` |
| `time_bg_door_d2_open` | `3ca85710ae2ba08ed9a2a6b50c597375021537c4bf68e9b540a04f93c3f6bfa1` |
| `time_bg_door_d2_clos` | `8a0357e0bc6a877eefe48c8fbd08b39d1b6d256cfc52501b4e24bb51c86e8f96` |

The strings `sdef_door_l_open` and `sdef_door_l_clos` refer to sound
definitions reached by `LaySEClip`; they are not scheduler aliases. The same
applies to the dock-resource strings `sdef_door_a_open/clos` and
`sdef_door_b_open/clos`.

The actor-packet switch at `0x0058cca0` handles the associated map-object
messages. Packet `0x00d8` creates a binding from the instance ID at packet
offset `+0x10` and layout ID at `+0x14`. Packet `0x00d9` resolves an animation
through that binding. The helper at `0x00585320` copies four name bytes and
then writes a terminator, matching `open` and `clos` exactly.

## Docked-ship boundary

Bindings `196/456` and `496/456` expose `spin`, `spot`, `_ex_show`,
`_ex_hide`, `_in_show`, `_in_hide`, `vst1`, `vst2`, `vst3`, and `set0` from
`sgrp_teikisen`. They do not expose a standalone `open` or `clos` alias.
Their resources contain eight `time_bg_door_a*` and `time_bg_door_b*`
timelines, and the ship sequences invoke nested schedulers, but that proves
only authored internal capability. It does not prove that a caller can safely
address an individual dock-side door.

The dock decoder also reports `unexpected collision body size` for
`time_bg_door_b1_open` and `time_bg_door_b2_open` in both dock resources.
Those four timelines remain only partially decoded. Their close counterparts
and all A-door timelines decode, but the successful neighbors do not justify
inventing the missing collision records.

## Scripted map-object selectors

The 1.23b `MapObjShipPort` script is LPB
`729s9/wu7/x9uv80/x9uv80r21uuvsq.le.lpb` (SHA-256
`01a5ae6466cfe2717c22d22eb4e491c3942bd5b6e53e64ffc4a688e448c5267f`);
its 1,044-byte decoded payload has SHA-256
`74ff78c013cb389e77314f77f00cdb5ce8d7d065889d1f35e1b78ca55e755fb8`.
The matched script is
`lua/scripts/chara/npc/mapobj/mapobjshipport.lua` in
`xivl-client-scripts:manifests/retail_lua_coverage.json`.
`MapObjShipPort.initForEvent` uses its second argument `A1` to select a cycle
of 600 when `A1` is 196 or 496, and 300 otherwise. It reads
`worldMaster:_getServerTime()` once and takes the result modulo that cycle.
For `A1` values 131, 321, or 431, it calls `stt0` when the phase is in
`[cycle/2 - 40, cycle/2 - 20)`, passing `phase - cycle/2 + 40` clamped at
zero. It calls `end0` when the phase is in
`[cycle/2 - 10, cycle/2 + 10)`, passing `phase - cycle/2 - 10` clamped at
zero; that expression is negative throughout the tested interval, so the
passed value is zero. It makes no scheduler call for those three `A1` values
outside the two intervals. For `A1` 196 or 496, it calls `spot` in the first
half with the phase as its offset and `spin` in the second half with the
phase relative to the half-cycle. The remaining values use the reverse
`spin`/`spot` order. It calls `_setGroundOn(false)` after the selector.

The 1.23b `MapObjShipRouteLand` script is LPB
`729s9/wu7/x9uv80/x9uv80r21usvpq5y9w6.le.lpb` (SHA-256
`f18f8d6c5b5717108a14ef0a36d01082e8400e7884424731d8a725d69dde6298`);
its 756-byte decoded payload has SHA-256
`9fdbfe23fa56d493849fd09a83dcb40cfe1c121a47f20a04f703d04702bfc74e`.
The matched script is
`lua/scripts/chara/npc/mapobj/mapobjshiprouteland.lua` in the same coverage
manifest. `MapObjShipRouteLand.initForEvent` checks `A1 == 5145`; if true,
it tests `_getServerTime() % 600 < 240` and calls `fdot` with
`240 - _getServerTime() % 600` when that test passes. The value used for
the call is from a separate time read. For other `A1` values, it tests
`_getServerTime() % 600 >= 360` and calls `fdin` with
`_getServerTime() % 600 - 360` when that test passes, again using another
time read. It then calls `_setGroundOn(false)`.

These bodies establish the numeric selectors, branch intervals, scheduler
names, and arguments in the client initializer. They do not identify the
meaning of `A1`, the units used by `_getServerTime()` or the offset argument,
the route or endpoint, or visible playback. Their authored calls do not
establish that a scheduler ran in a historical session.

## Native midstream bridge

The pinned executable named above was mapped with pefile 2024.8.26 and decoded
with Capstone 5.0.7 for x86-32. Wrapper VA `0x00737F80` loads handler VA
`0x006F3BA0` into its setup sequence; the wrapper also passes the string at
`0x00FD6F2C`, whose bytes spell `_runBgSchedulerFromMidstream`. The helper
contracts in that sequence are not assigned here.

In the handler, argument index 1 is read as a 32-bit float at
`0x006F3C21-0x006F3C33`. Instructions at `0x006F3C3B-0x006F3C45` multiply
that value by the double at `0x00F91C48` (`1000.0`). The following conversion
sets the x87 rounding mode to truncate toward zero and stores an integer with
`fistp`; the low 32 bits proceed into later member-container work. This
establishes the native numeric transformation, not the argument's semantic
unit, a milliseconds or ticks interpretation, server scheduling, or visible
playback.

## Summer-named strings

The resources at `0x89ED0003` and `0x89ED0004` each contain the
literal names `time_bg_smmr_show` and `time_bg_smmr_hide`:

| DAT key | String | Offset |
| --- | --- | ---: |
| `0x89ED0003` | `time_bg_smmr_show` | `0x500F` |
| `0x89ED0003` | `time_bg_smmr_hide` | `0x5021` |
| `0x89ED0004` | `time_bg_smmr_show` | `0x4F06` |
| `0x89ED0004` | `time_bg_smmr_hide` | `0x4F18` |

| DAT key | DAT SHA-256 |
| --- | --- |
| `0x89ED0003` | `d1f86185bc09c0f5b0875c2a85052685cdab8c295b1dc46c68eae928d2999b47` |
| `0x89ED0004` | `47eb8f538bda6d354f7595e168d9ba6a5d7d901acde594021e1ed8fca4abc5df` |

These offsets establish resident names only. They do not identify an event,
owner, call route, or historical activation.
The source inventory is `support_resource_scheduler_strings.csv` rows 9-12;
the layout-key associations are listed in `region_resource_all_rows.csv`
rows 961-962. The strings and offsets were checked against the DAT
bytes.

## Evidence identity and limits

| Layout | DAT SHA-256 |
| ---: | --- |
| 196 | `35f5df6d3138398f8b6fcba0025b4a870eb8370cbaaabad423ae8da4f17fa4db` |
| 496 | `fbd36f5d2fa3b9681815d52ecd4030d274bcc2f837589186a29cba1a2d96c98d` |
| 5142 | `d1f86185bc09c0f5b0875c2a85052685cdab8c295b1dc46c68eae928d2999b47` |
| 5143 | `47eb8f538bda6d354f7595e168d9ba6a5d7d901acde594021e1ed8fca4abc5df` |

The resources prove layout ownership, callable aliases, authored transforms,
sound clips, and collision timing. They do not supply the server schedule,
proximity policy, active route, actor spawn mapping, or historical runtime
selection. Those behaviors need independent server or capture evidence; they
must not be inferred from resource names or mirrored coordinates.
