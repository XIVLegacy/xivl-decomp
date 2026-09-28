# Rivenroad command and presentation contract

The FFXIV 1.23b command tables, m917 and m091 resources, client Lua, and arena
layout preserve a substantial Rivenroad presentation contract. They do not
contain the lost server encounter state machine, target policy, damage values,
or phase thresholds.

## Command geometry

The command rows were decoded from `data/01/03/00/97.DAT` and
`data/01/03/04/A6.DAT`, whose SHA-256 values are
`54a8221c7d542793879c6c57d1ed259e5046fd5eed04495f55f502e220801790`
and `1d11651de5a1a3f0479f702b9c8df0ad02b50b7c33c8963f26ff359da96a3a11`.

| ID | Native name | Range or shape | Channel | Cast |
| ---: | --- | --- | --- | ---: |
| 23596 | Megaflare | radius 50 | Fire | instant |
| 23597-23602 | meteor charge, cancel, and warp helpers | non-damaging helper rows | none | instant |
| 23603 | Thermionic Beam | range 30 | Lightning | instant |
| 23604 | Thermionic Burst | radius 2 | Lightning | instant |
| 23605 | Plasma Acoustics | range 25 | physical | 3.5 s |
| 23606-23607 | Dive charge and jump helpers | non-damaging helper rows | none | instant |
| 23608 | Dalamud Dive | radius 50 | Fire | instant |
| 23609 | interrupted Dive presentation | no damage | none | instant |
| 23610 | local Dive strike | radius 2 | Lightning | instant |
| 23611 | Iron Chariot | radius 12 | physical | instant |
| 23612 | Lunar Dynamo | annulus 15-25 | Astral | 1.0 s |
| 23613 | Ravensbeak | range 8 | physical | instant |
| 23614 | Chaos Thrust | range 8 | physical | instant |
| 23615 | Twisting Vice | range 8 | physical | instant |
| 23616 | lunar-fragment explosion | radius 50 | Fire | instant |
| 23617 | Aetherial Emission | range 30 | heal property | instant |
| 23624-23626 | Megaflare variants | radius 50 | Fire | instant |
| 23627 | Lunar Dynamo variant | annulus 15-25 | Astral | 3.0 s |
| 23631 | Fierce Ravensbeak | range 15 | physical | instant |
| 23644 | meteor cleanup explosion | no damage | none | instant |
| 23645 | meteor explosion variant | radius 50 | Fire | instant |

The annulus on Lunar Dynamo is direct minimum-range and range evidence. A
radius row does not independently prove target choice or when the server
evaluated positions. Likewise, helper rows with no damage do not establish
phase ordering.

## Nael and fragment resources

Nael uses model family m917. The sources include BID banks 0000 and
0001 plus WSS banks 0001-0007, 0009-0012, and 0014-0019. Selected immutable
identities are:

| Resource | Bytes | SHA-256 |
| --- | ---: | --- |
| m917 BID 0000 | 800,448 | `7878c52b1829d12002f1394704976d57d4df72a152228a3417502566dfe4f571` |
| m917 BID 0001 | 666,496 | `3d0d348e0458baadf954f8f9e58d23647b9c747b56311f3eac10842f9058878e` |
| m917 WSS 0001 | 558,912 | `43267adc25a5cc5222c1975c7a21a335cdbabf48955d22dbd60b6c034c537139` |
| m917 WSS 0010 | 296,560 | `ceb64d20125ad0bb57887655205e0b896ba4a3d309aa4271da25a2cfa73e53ff` |
| m917 WSS 0015 | 559,040 | `b7d794582af055bf37cb103c05d27ac31993f09576049f224acf6396fa1c718f` |
| m917 WSS 0018 | 717,584 | `0c901e313e85c4c4901e66d6ce2e35d82a2620eaed2ea404280d93b69c42b848` |

Effect names and scheduler structure provide strong presentation joins:

| WSS | Supported presentation |
| ---: | --- |
| 1 | Megaflare wing/effect sequence |
| 5 / 6 | warp-in / warp-out sequences |
| 7 | Thermionic Beam |
| 9 | Plasma Acoustics |
| 10-12 | Dive charge, resolution, and cancellation family |
| 14 | Iron Chariot |
| 15 | Lunar Dynamo |
| 16 | Ravensbeak |
| 18 / 19 | Chaos Thrust / Twisting Vice candidates from ordinal and authored Japanese weapon-skill tokens |

The WSS18 and WSS19 command association remains an inference. Bank existence,
ordinal proximity, and a translated resource token are not a recovered dispatch
table.

Lunar fragments use m091. Its WSS 0001 is 484,912 bytes with SHA-256
`2d24cd3999f7f883ed08521bd87d74f711a212c09dc4a6f2de576861df139426`.
The recovered `WhiteGeneralMeteor.initForBattle` method calls
`_setGroundOn(false)`, proving the fragment actor is configured off ground.
The script does not supply its descent route, duration, impact threshold, or
server damage.

Most recovered White General, Golem, Lentigo, and weapon-skill client classes
are identity-only. `InstanceRaidLesserWhiteGeneral.processStartEvent` calls
`executeCutScene("gc010715", ...)`; its dynamic cutscene method fades out,
sets a server-provided weather argument, executes a server-provided scene key,
and fades in. The normal/hard director identity does not reconstruct either
encounter.

## Arena layout

The Rivenroad layout is `data/AB/F4/00/00.DAT`, 67,904 bytes,
SHA-256
`8dbf9335b4f357421a6dab2dc349bcd5009d2760c7f421d9b52ffaf6c57fb830`.
It contains 13 layout instances and 48 nested members. The root group is
rotated 30 degrees and owns two symmetric island-and-stair sets. Upper glyph
VFX members are authored at Y `21.204000473`.

That Y value and the symmetric structure are direct layout evidence. Exact
middle-platform collision, stair paths, server spawn points, and encounter
movement are not serialized as an immediately equivalent table of world
positions. They must not be derived solely from visual symmetry.

## Parser boundary

One decoded shape mismatch is intentionally retained: m917/e001 resource
`s_awl_on` has an `EffectEnd` target which resolves to
`RaptureChantSyncClip`, not an `ActionClip`. The mismatch must not be silently
dropped or rewritten to fit the common ActionClip-target pattern.

## Evidence boundary

The client proves command scalar fields, resource availability, selected
effect-name joins, fragment ground state, the start-scene literal, and layout
geometry. It does not prove HP, damage, phase order, fragment or golem counts,
entry rules, retries, rewards, ordinary cadence, descent timing, target
selection, or exact world placement. Historical guides and video may support
some of those questions, but they remain separate evidence rather than client
decompilation.
