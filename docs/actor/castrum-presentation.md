# Selected Castrum presentation resources

The selected retail m525 model files embedded 12 SCB schedulers. The selected
m052 action banks contained 18 SCBs and 28 MTB motion containers. These
resources established serialized presentation capability; their presence did
not identify a caller, F-1/H-1 variant, content13 spawn, or authoritative
battle state.

## Selection

`xivl-client-data:docs/actor-appearance-crosswalk.md`, section Selected
generator, transmitter and Vanguard rows, at revision
`7ccdc281b1efd0064cb14a27590cb96589b7086a` owns the eight selected actor-table
identities. The [appearance path decoder](appearance-path-decoder.md) maps the
candidate base and packed gear fields to m525 e001/e002 and m052 under its
native input conditions. That conditional path motivated this resource
selection; it does not establish a live actor's selected appearance or Lua
class. The separate [Tower
layout](../resource/transmission-tower-rendering.md) owns the shutter and zone
presentation.

## Evidence identity

Paths below are relative to the retail install root. SHA-256 covers the
complete original file, including the PWIB continuation.

| File | Bytes | SHA-256 |
|---|---:|---|
| `client/chara/mon/m525/equ/e001/met_mdl/0001` | 395152 | `403a1d907f55bc91ab1fc3b2a1bb5c859d1b4beac94e4f65eeddf8217ba899a9` |
| `client/chara/mon/m525/equ/e001/top_mdl/0001` | 143968 | `3721ab44b0a6d8b0ebe964dd7dd3989f43d92eb613ac5f64825becc4f2a6a59f` |
| `client/chara/mon/m525/equ/e002/met_mdl/0001` | 296816 | `18433d3338c171423afd3ead555d3d2c8183a74988728793e87d6ba66d2b855b` |
| `client/chara/mon/m525/equ/e002/top_mdl/0001` | 106736 | `f245d2fe3b583cbe79f5fd6b5410d1c88a97195f2578a8b7ce19faa137f5ceaf` |
| `client/chara/mon/m052/act/emp_emp/bid/base/0000` | 311760 | `0ddecf464e70fefe6d8648908300c756677083cc0f6a6fcf53f8a9c25ef22d0b` |
| `client/chara/mon/m052/act/emp_emp/wss/base/0001` | 185944 | `afadbb04d258203fde544c303b76aed3c8498e77938cf07df8910d53db374331` |
| `client/chara/mon/m052/act/emp_emp/wss/base/0002` | 323032 | `ed19d88f50be6ccbd0b9902d4b73b0939964ca7db1ff67168b07075f161e0a26` |
| `client/chara/mon/m052/act/emp_emp/wss/base/0003` | 425632 | `77f04eaedc28eb7b2d340c9e694cd31229e5fb0622e571b3f541ca7a39caf9b5` |

## Embedded scheduler and motion records

| Selected file under client/chara/mon | Bounded contents |
|---|---|
| m525/equ/e001/met_mdl/0001 | 10 SCBs and two nested effect RES directories; SCBs at 0x130, 0x600, 0x21018, 0x21438, 0x21858, 0x21c78, 0x32444, 0x32834, 0x32d24, 0x33274 |
| m525/equ/e002/met_mdl/0001 | Two SCBs at 0x26750 and 0x26bc0, named init_msb4_0 and init_msb4_1; one nested effect RES directory |
| m525/equ/e001/top_mdl/0001 and e002/top_mdl/0001 | trb/cibe resources with nested sdrb/wrb/skl/trb entries; no bounded SCB found in these selected RES trees |
| m052/act/emp_emp/bid/base/0000 | 25 MTB, 25 MCB, 15 CIBT and one CIBC entry; nested RES at 0x47e80 containing nine SCBs |
| m052/act/emp_emp/wss/base/0001 | MTB cbbm_sp_a01 at 0xe0, three SCBs at 0x3ca8, 0x4438, 0x4a18, and a nested effect RES |
| m052/act/emp_emp/wss/base/0002 | MTB cbbm_sp_a02 at 0xe0, three SCBs at 0x4fe8, 0x5778, 0x5db8, and a nested effect RES |
| m052/act/emp_emp/wss/base/0003 | MTB cbbm_sp_b01 at 0xe0, three SCBs at 0x3e7d0, 0x3ef00, 0x3f380, and a nested effect RES |

The e001 met_mdl idle/dead-labelled schedulers each contained BindActorClip,
ClipSyncClip and RaptureCasterManagedSchClip. Its init_msn000..003 labels
contained 0/1/2/3 ActionClip and 0/1/2/3 KillClip records. The serialized
labels did not establish gameplay phases or the conditions selecting them.

The bid schedulers carried dead, cas00, cas90, casa0, casb0, itm00 and three
cast_mon labels. Each wss bank carried a byte-identical shoot_mon main SCB
(SHA-256 7da7c8b13a2096a23de59d6719c2e2cad385cb8e6e295974273b33e7a08c0422), a
skill-labelled mon_main and an m052_0001/0002/0003 scheduler. Their class
tables included MotionClip, sound/effect, movement-stop, status-lock and
selection classes. All 30 SCBs had one ProxyActor record and all 223 clips
selected actor ordinal 0. Class names did not prove execution; raw slot-7
values were not interpreted as encounter timers.

Across all 223 records, `s16(D+0x0c)` equalled the record's zero-based
position in its SCB's all-block clip order. The
[clip-index contract](../resource/cut-scheduler-clip-timing.md#clip-index-word)
uses this word for built-list lookup. This authenticated index alignment does
not establish a selected block, callback invocation or encounter timing.

All 28 MTBs had version 0x43, Header and SpuBinary section names, three group
names (root, 005_n_hara, other), and three motion pair-table entries. Their
big-endian SpuBinary blobs contained 58 descriptors and 4212 channels.
Quaternion descriptors used kind 0; scalar descriptors used kinds 4..9 and
11..16. Track lengths equalled the sum of their bounded channel spans. The
runtime row-name mapping was not supplied by these files, so joint keys were
not assigned skeleton-node names or rendered poses.

## MotionClip names and resident MTB entries

The [combat MotionClip name provider](../resource/combat-motion-name-provider.md)
qualifies the global registry's RaptureMotionClip factory and its RIDTBL
lookup. Under that construction path, when the callback context retains the
corresponding scheduler, `s32(D+0x10)` selects a 16-byte name entry from that
scheduler's RIDTBL. It is not a String-table index. The selected callback
context and actual invocation have not been established.
The selected entries below are NUL-terminated within 16 bytes, so the
provider's 15-byte output limit does not shorten them.

The four m052 files above contain seven MotionClip records. Offsets in this
table are absolute file offsets. Bank abbreviations are relative to
`client/chara/mon/m052/act/emp_emp/`, with `base/` omitted.

| Bank | Scheduler label | Clip D | RIDTBL index | Name entry | Selected name | Same-bank MTB offset |
|---|---|---:|---:|---:|---|---:|
| bid/0000 | dead | 0x4821c | 1 | 0x482e0 | cbbm_ded | 0x19328 |
| bid/0000 | cas00 | 0x4874c | 1 | 0x487f0 | cbbm_abl_1 | 0xe1d8 |
| bid/0000 | cas90 | 0x48c2c | 1 | 0x48cd0 | cbbm_abl_1 | 0xe1d8 |
| bid/0000 | itm00 | 0x4992c | 1 | 0x499d0 | cbbm_abl_3 | No match in this bank |
| wss/0001 | skill01/mon_main | 0x46e8 | 0 | 0x4808 | cbbm_sp_a01 | 0xe0 |
| wss/0002 | skill02/mon_main | 0x5a28 | 1 | 0x5b88 | cbbm_sp_a02 | 0xe0 |
| wss/0003 | skill3/mon_main | 0x3eb58 | 2 | 0x3ec90 | cbbm_sp_b01 | 0xe0 |

Each MTB match is equality with the final component of an original RES
directory name in that same bank. The BID entries are under
`mon/m052/animation/a001/bt_emp_emp/miga/base/b001/bin/`; the WSS entries
have an additional leading `chr/`. The MTB sizes are 19495 bytes for
cbbm_ded, 7687 for cbbm_abl_1 and 14647/19575/16455 for the three WSS
entries respectively. These are authored name/resource associations. The
native motion-resource search, loaded-bank selection and successful playback
remain separate from this equality. The selected BID bank's lack of
cbbm_abl_3 does not establish its absence from other resources.

The six matched records also have same-bank MCB and CIBT entries with those
same final-component names. Original RESOURCE_TYPE values are `0x006D7462`
for MTB, `0x006D6362` for MCB and `0x63696274` for CIBT. File offsets and
payload byte counts for the companion entries are:

| Bank | Name | MCB offset / bytes | CIBT offset / bytes |
|---|---|---|---|
| bid/0000 | cbbm_ded | 0x1df50 / 760 | 0x1e248 / 44 |
| bid/0000 | cbbm_abl_1 | 0xffe0 / 592 | 0x10230 / 64 |
| wss/0001 | cbbm_sp_a01 | 0x3a18 / 592 | 0x3c68 / 64 |
| wss/0002 | cbbm_sp_a02 | 0x4d58 / 592 | 0x4fa8 / 64 |
| wss/0003 | cbbm_sp_b01 | 0x4128 / 592 | 0x4378 / 64 |

The [native request](../resource/motion-resource-request.md) first tests
MCB availability through the actor's current resource provider before querying
MCB, MTB and CIBT again. These authored companion entries do not identify that
provider or prove lookup success. The selected BID bank has no MCB or CIBT
name match for cbbm_abl_3 either. The companion payloads were not decoded here.

All seven records are 0x2c bytes. They have `u8(D+0x14)=0x20`,
`u8(D+0x15)=1`, `s32(D+0x1c)=0`, `u16(D+0x24)=100` and
`u8(D+0x26)=1`. The remaining request words are:

| Scheduler label | u32(D+0x18) | s32(D+0x20) |
|---|---:|---:|
| dead | 1050000 | 1050000 |
| cas00, cas90 | 300000 | 300000 |
| itm00 | 750000 | 750000 |
| skill01/mon_main | 700000 | 700000 |
| skill02/mon_main | 900000 | 900000 |
| skill3/mon_main | 800000 | 800000 |

The [RaptureMotionClip consumer](../resource/cut-scheduler-clip-payloads.md#motionclip)
owns the field conversions and request call. These raw values do not establish
encounter timers, cast times or server commands. The other eleven selected
m052 SCBs have no MotionClip record.

Reproduction reauthenticated the four complete files against the identities
above. Python 3.12.0 standard-library reads applied the native RIDTBL formula
to each selected SCB, checked name bounds and record identities, and compared
the original RES directory names, types, offsets and sizes. The public
`tools/scb_clip_scheduler.py` parser independently agreed with the retained
class and clip boundaries. No target execution supplied these observations.

## Parsing boundary

The [SCB layout](../resource/cut-scheduler-clip-layout.md), [MTB
framing](../resource/mtb-container-framing.md) and [motion blob
layout](../resource/mtb-fixed-motion-calculator.md) supplied the decoding
contracts. `xivl-tools:docs/formats/gtex-pwib.md`, PWIB contract and Directory
and type selection, supplied the split-container and RES directory rules.
Python 3.12.0 standard-library readers used the PWIB first span as the RES
view, rather than treating its inherited SEDB declaredSize as a standalone
container length. RESOURCE_TYPE supplied types by physical directory index;
nested entries stayed inside their parent payloads.

Nine metadata declarations crossed their owning views. The outer RESOURCE_ID
entries in e001/top_mdl, e002/met_mdl and e002/top_mdl exceeded their first
spans by 16 bytes. Each top_mdl nested trb RES had a RESOURCE_ID overrun of 51
bytes, and its next nested RES had RESOURCE_TYPE/RESOURCE_ID overruns of 11/75
bytes. The required RESOURCE_TYPE prefix remained bounded; every nonmetadata
child fit. The excess metadata bytes were not borrowed from adjacent payloads
or the continuation. Their meaning remains unresolved, and a strict all-entry
extent parser would reject these declarations.

Validation reauthenticated all eight files, checked directory associations,
scheduler class/actor/record bounds and motion descriptor/channel bounds, and
rejected 32 deliberate in-memory corruptions of split offsets, RES child
pointers, clip class/length fields and motion pointers. The continuation,
opaque model/effect leaves and undocumented clip payload bindings were not
decoded. These results did not establish runtime class, command, state, timer
or encounter selection.
