# Cast selectors and cast schedulers

A staged action selector with a high byte of `0x6F` or above, other than
`0x78`, does not go through the bank category table. The client turns its low
12-bit lane into a name. This page records that name lookup, the cast
schedulers of the installed resident bank and the schedulers of selected
installed reaction banks. The [action queue](action-queue.md#staged-type-and-packed-selector-domains)
owns the selector split and the earlier dispatch.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
base `0x00400000`. All native addresses are VA. `ghidra` CLI driving Ghidra
`12.1.4` produced the disassembly. Tables and strings were read from the
file-backed PE sections.

Install measurements walk the RES directories of
`xivl-tools:docs/formats/sedb-res.md`, nested RES entries included, and parse
each `SEDBSCB` child with `tools/scb_clip_scheduler.py`. A clip start frame
is `u32(D+4)` over 10000 ticks
([clip timing](../resource/cut-scheduler-clip-timing.md#time-unit)). A
MotionClip end frame is `u32(D+0x18)` over 10000. A motion name is a `cbbm_`
string inside the span of the scheduler entry.

## Name dispatch

STATIC. In `0x00798470` a high byte of `0x78` is handled first. Another high
byte of `0x6F` or above (`0x0079854E..0x00798551`) needs a nonzero queue
backpointer at visual `+0x1CC`. It calls visual virtual `+0x60` with the high
byte, the low 12-bit lane and a zeroed out pointer
(`0x00798564..0x0079857A`). A null result ends the route. Otherwise it calls
virtual `+0x1C` of the queue object with that name and the result of visual
virtual `+0x4C` (`0x00798582..0x007985A0`). When that call returns 0 and the
out pointer was set, the same call is made once with the out name
(`0x007985A4..0x007985D3`). The body of queue virtual `+0x1C` was not traced
here.

STATIC. The concrete visual vtable `0x00FE4544` holds `0x00798A40` at cell
`0x00FE45A4`. That method routes on the high byte:

| High byte | Name routine |
| --- | --- |
| `0x6F` | `0x00798660` |
| `0x70` | `0x007986C0` |
| `0x71` | `0x00798720` |
| `0x7E` | `0x00798780` |
| every other value | `0x007987F0` |

STATIC. `0x00798660` reads one 12-byte record at `0x012C44D4`: first lane 0,
array pointer `0x012C44E0`, count 20. `0x007986C0` reads the record at
`0x012C4530`: first lane 0, array pointer `0x012C4540`, count 20. The named
lanes are:

| Lane | `0x6F` name | `0x70` name |
| --- | --- | --- |
| 1 | `cast_hou_1` | `cast_hou_2` |
| 2 | `cast_jyu_1` | `cast_jyu_2` |
| 3 | `cast_gen_1` | `cast_gen_2` |
| 4 | `cast_sou_1` | `cast_sou_2` |
| 5 | `cast_ws_1` | `cast_ws_2` |
| 7 | `cast_itm_1` | placeholder |
| 8 | `cast_son_1` | placeholder |
| 11 to 19 | `cast_mon_11` to `cast_mon_19` | `cast_mon_21` to `cast_mon_29` |

Every other lane below 20 holds a pointer to a placeholder string, which reads
`*` in each entry of the two arrays.

STATIC. `0x007987F0` walks eleven 12-byte records from `0x00FE4498`. Each
holds a u16 first lane, a flag byte, a u32 array pointer at `+4` and a u32
count at `+8`. The last record has first lane 0, array `0x012C4708` and count
100. Its names include:

| Lane | Name |
| ---: | --- |
| 1 | `dk_only` |
| 2 | `cast_stop_1` |
| 11 | `guard_on_1` |
| 12 | `guard_off_1` |
| 22 | `cast_stop_2` |
| 31 | `guard_on_2` |
| 32 | `guard_off_2` |
| 98 | `put_mes` |
| 99 | `stop_mcast` |

INFERRED. The `0x6F` family starts a cast and lanes 2, 22 and 99 of the
generic table stop one. Which server message sends each selector is not
established by these tables.

## Resident cast schedulers

MEASURED. `data/15/D9/00/00.DAT` (4973264 bytes, SHA-256
`10b8d7dc501a14a640e8338b0e17d2cc521756583fe4ed68859071d004b453e6`), the
[resident bank](hit-damage-schedulers.md#resident-id-list), holds
`sch\neri\cast_gen_1`, `cast_jyu_1`, `cast_hou_1` and their `_2` forms.
`cast_gen_1` and `cast_jyu_1` run these clips:

| Frame | Clips |
| ---: | --- |
| 0 | BindActorClip, RaptureCasterSchClip `cas00` |
| 5 | RaptureActionSubStatusSchKickClip, RapturePutActionLogClip |
| 10 | RaptureSoundClip, ActionClip |
| 12 | RaptureChantSyncClip |
| 14 | RaptureSoundClip, RaptureEffectEndClip |
| 20 | IfClip |
| 37 | RaptureCasterSchClip `shoot_gen_1` or `shoot_jyu_1`, RaptureCharaActionSoundClip |
| 58 | ClipSyncClip |

`cast_hou_1` has the same list without the two sound clips and starts
`shoot_hou_1`. Each `_2` form of these three names `cas01` in place of `cas00`
and starts the same `shoot_` name as its `_1` form. `shoot_gen_1` and
`shoot_jyu_1` hold BindActorClip, RaptureSoundClip and ActionClip at frame 0.
`shoot_hou_1` omits the sound clip.

`cast_stop_1` starts RaptureCasterManagedSchClip `sht_can0` at frame 0, holds
RaptureCancelChantSyncClip entries for `cast_jyu_1`, `cast_gen_1`,
`cast_son_1`, `cast_sou_1`, `cast_abi_1` and `cast_ws_1` at frame 0 and a
RaptureActionSubStatusSchKickClip at frame 20. `cast_stop_2` starts
`sht_can1` and names the `_2` forms except for `cast_son_1`. `stop_mcast`
holds RaptureCancelChantSyncClip entries for `cast_mon_` names, a
RaptureActionSelectClip and a RaptureActionSubStatusSchKickClip, all at
frame 0, and no caster clip.

## c001 reaction bank schedulers

MEASURED. These are entries of `client/chara/pc/c001/act/<set>/bid/base/0001`.
Each `cas` scheduler holds BindActorClip, RaptureClientMoveStopClip,
RaptureServerMoveStopClip and one MotionClip. Each `sht_can` scheduler holds
BindActorClip and one MotionClip. The end is the MotionClip end frame. The
inputs for this section are:

| Set | Bytes | SHA-256 |
| --- | ---: | --- |
| `emp_emp` | 250640 | `75ba240d4c5be143f615dbefd9efcc53c2478cb29c46b38f4b48c8b5e9636154` |
| `clw_clw` | 287184 | `2d947f2c4e5361e2d0febc6f8b297d9c4b1eef76e3b8f5b32ed67e54beca3182` |
| `stf_emp` | 304336 | `7d40543495e47e74b1dbab4ab2b873bf77bebd1d8598630012b066f0cf14f4f3` |
| `swd_emp` | 366288 | `0a5b2b26fcb5fe3f20262d2cd33411fd2cfb73e9135c15a023e61adba3e4f90a` |

| Scheduler | Motion | `emp_emp` end | `clw_clw` end | `stf_emp` end |
| --- | --- | ---: | ---: | ---: |
| `cas00` | `cbbm_abl_1` | 30 | 40 | 30 |
| `cas90` | `cbbm_abl_1` in `emp_emp`, `cbbm_wslp_1` in the others | 30 | 30 | 30 |
| `sht_can0` | `cbbm_abl_3_can` | 52 | 90 | 90 |
| `cas01` | `cbbm_abls_1` | absent | absent | 30 |
| `sht_can1` | `cbbm_abls_3_can` | absent | absent | 45 |

A motion name in a scheduler does not imply a matching MTB entry in the same
bank. `emp_emp` holds MTB entries `cbbm_abl_3_can` and `cbbm_abls_1` but no
`cas01`. `clw_clw` and `stf_emp` hold neither `cbbm_abl_3_can` nor
`cbbm_abls_1`. The `swd_emp` bank has `cas01` and no `cbbm_abls_1` entry.
The `emp_emp`, `clw_clw`, `stf_emp` and `swd_emp` banks each hold an MTB entry
`cbbm_abl_2lp`. No `cas` or `sht_can` scheduler of these four banks names it.

## m011 reaction bank

MEASURED. `client/chara/mon/m011/act/emp_emp/bid/base/0000` (106288 bytes,
SHA-256 `480cdf5197e8f8be89d3ad3625c964b5d87761b625a439dc275efd4c9ca42a9c`)
holds `cast_mon_11` to `cast_mon_14`. `cast_mon_11` starts the caster
scheduler `casb0` and the other three start `casa0`. Both are entries of the
same bank and hold BindActorClip, RaptureClientMoveStopClip and
RaptureServerMoveStopClip with no MotionClip. The bank holds an MTB entry
`cbbm_abl_2lp`.

## Open

- The body of queue virtual `+0x1C`, and which bank answers a cast scheduler
  name at run time.
- What requests the `2lp` loop motions and what ends a cast pose.
- The routines `0x00798720` and `0x00798780`.

## Reproduction

```text
ghidra disasm 0x00798470 -n 140
ghidra disasm 0x00798A40 -n 30
ghidra disasm 0x00798660 -n 30
ghidra disasm 0x007986C0 -n 30
ghidra disasm 0x007987F0 -n 50
```

Read the three dwords at `0x012C44D4` and `0x012C4530`, the 20 string
pointers of each array, the eleven records at `0x00FE4498` and the cell
`0x00FE45A4`.
