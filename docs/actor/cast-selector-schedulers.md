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

### Chant wait clips and the branch

MEASURED. The clip indices below are zero based in record order. In
`cast_gen_1` the ActionClip is clip 5 and the RaptureChantSyncClip is clip 6.
The RaptureEffectEndClip at frame 14 holds a count of 1 at `D+0x10` and the
`s16` index 5. In `cast_hou_1` it holds index 4, which is that scheduler's
ActionClip.

MEASURED. The [IfClip record](../event/grand-company-mission-bytecode.md#ifclip-record)
of `cast_gen_1` names track 5 and has three branches. Each has combine mode 0
and one condition: operator 1, type 0, the index of the ChantSync clip and a
value. Value 1 targets track 6, value 2 track 8 and value 3 track 9. The frame
37 and frame 58 clips store 7 in the `s16` at `D+0x08`. INFERRED. Reading
that field as the clip track, and the `@CTRK` rows as parent indices that put
track 7 under track 6, value 1 alone enables the `shoot_` clips. The `@CTRK`
row layout was not established from a consumer.

MEASURED. The sound clips of the cast schedulers each store 1 at `D+0x14`,
volume 1.0 at `D+0x1C`, 0 at `D+0x20` and one resource ID at `D+0x28`. The slot
is `D+0x18`. The
[RaptureSoundClip consumer](../resource/cut-scheduler-clip-payloads-ranks-14-22.md#rapturesoundclip)
owns those fields. Each scheduler below starts one clip at frame 10 and one at
frame 14 in the same slot. The frame 14 resource is `0x07FE0007` in every row.

| Schedulers | Slot in `_1`, `_2` | Frame 10 resource |
| --- | --- | --- |
| `cast_abi_1`, `cast_abi_2` | 1, 2 | `0x615E0000` |
| `cast_gen_1`, `cast_gen_2` | 1, 2 | `0x615E0001` |
| `cast_jyu_1`, `cast_jyu_2` | 1, 2 | `0x615E0002` |
| `cast_ws_1`, `cast_ws_2` | 1, 2 | `0x615E0003` |
| `cast_son_1` | 1 | `0x615E0009` |

`cast_hou_1`, `cast_hou_2`, `cast_sou_1` and `cast_sou_2` hold a
RaptureChantSyncClip and no RaptureSoundClip. `shoot_gen_1`, `shoot_jyu_1`,
`shoot_ws`, `shoot_abi` and `shoot_song` each hold one sound clip at frame 0 in
slot 0 with resources `0x615E0005`, `0x615E0007`, `0x615E0008`, `0x615E0004`
and `0x615E000A`.

MEASURED. No other resident scheduler whose name starts with `cast` or `stop`
holds a RaptureSoundClip. These include `cast_tele1`, `cast_stop_1`,
`cast_stop_2` and `stop_mcast`. So none of the `cast` or `stop` schedulers of
the bank holds a slot 0 sound clip.

MEASURED. The headers below follow the
[SSCF stream header](https://github.com/XIVLegacy/xivl-tools/blob/bb76ad408548a997fec6ac8809970d9ea7d84832/docs/formats/sscf.md).
Each named `0x615E` resource holds one mono stream with codec ID 12. Its codec
head has format tag 2, block align 70 and 128 samples per block. The streams of
the frame 10 resources have rate 44100.

| Frame 10 resource | Loop start | Loop end |
| --- | ---: | ---: |
| `0x615E0000` | 123904 | 285696 |
| `0x615E0001` | 125824 | 408704 |
| `0x615E0002` | 80640 | 306560 |
| `0x615E0003` | 106752 | 197120 |
| `0x615E0009` | 72960 | 360832 |

Each loop end in the table equals the codec data length divided by 70 and
multiplied by 128. INFERRED. For these five streams the loop fields count sample
frames. This does not settle the codec 12 unit for other containers. Both loop
fields are 0 in the five `shoot_` resources above.

The frame 14 resource `0x07FE0007` is 400 bytes, SHA-256
`b673e724a9bba24588030eb122c06638e7a0bcc4cdfa7ad57a250661559c737d`, with a
stream count of 0. The
[music table](../resource/music-song-table.md#stream-less-containers)
records how the client treats such containers on its song path. INFERRED. The
frame 10 resource is the chant loop, and the frame 14 clip ends it by taking its
slot. What a slot play does to a sounding resource was not traced.

## Chant wait and release

STATIC. RaptureChantSyncClip has vtable `0x01017554`. Slot 1 `0x008227A0` sets
the clip phase to 1 through `0x00A210F0`, which
[holds the scheduler clock](../resource/cut-scheduler-clip-timing.md#phase-word).
Slot 2 `0x008228A0` takes the dword at `+0x0C` of its argument (`0x00A20070`)
and forms the member address `+0xF0` in it. A null address stores the clip
value 1 and sets phase 0. Otherwise it calls virtual `+0x6C` of that member and
reads the pointer the result holds. The clip stays when that pointer is null or
its dword at `+0x04` is not `0x00766678`. It also stays while the object's
dword at `+0x48`, read by `0x0080A800`, is 0. A nonzero dword becomes the clip
value through virtual `+0x38` (`0x00A21110`, `s16` at clip `+0x30`), which the
IfClip conditions read, and the phase returns to 0. The slot then resets the
chant dword to 0 at `0x00822941` when byte `+0x04` of the clip virtual `+0xA0`
result is 1 and record byte `D+0x10` is nonzero.

STATIC. The direct callers of the setter `0x0080BCD0` are `0x0081DC47` in
`0x0081DBD0`, `0x00820307` in `0x00820290`, `0x00634AA6` in the thunk
`0x00634A80`, the reset above, the queue join at `0x00845C3C` below and the
[bucket 12 control request](../resource/cut-status-and-parameter-clips.md#character-bucket-12-control-request)
at `0x00844FC2`, which writes 1. `0x0081DBD0` and `0x00820290` read a parent
chant value and write it to each listed child. Their addresses occupy the
listener cells `0x010117D0` and `0x0101240C`.

STATIC. RaptureCancelChantSyncClip slot 1 `0x00822BF0` requires byte `D+0x10`
to be 1. It takes the 16 byte name at `D+0x11` and calls virtual `+0x0C` of
the object at RaptureActor `+0x9C` with the actor and that name. In the
`Application::Scene::RaptureActorClipListener` table `0x00FB7AF4` that cell is
`0x007CE870`. It looks the named scheduler up and, when the returned holder is
nonnull, calls `0x00634A80` with 1. On this path a cancel therefore releases
the named chant with value 1. Which object a live actor holds at `+0x9C`, and
what keeps the `shoot_` track from showing after an interrupt, were not
traced.

### Queue join

STATIC. Within each of its 26 buckets,
[`0x00845430`](action-queue.md#controller-consumption) remembers an entry in
state 1 at entry `+0x04` that passes virtual `+0x2C` with 1. A later pending
entry in state 0 of the same pass that also passes it supplies its virtual
`+0x30` value to the remembered entry. Through the remembered entry's `+0x10`
object, virtual `+0xC8` and then virtual `+0x6C`, the routine reaches the
chant object and calls `0x0080BCD0` with that value (`0x00845C0A..0x00845C3C`).
Only one such write happens per bucket pass, and the pending entry that
supplied it is not activated by that arm. The `CharaActionQue` method behind
`+0x30` is `0x00843E20`. It returns the dword at `+0x08` of the object that
visual virtual `+0x4C` returns. Which wire routes store 1 in that dword was
not traced.

INFERRED. The join is per bucket, and a `0x6F` cast selector classifies as
[staged type 8](action-queue.md#staged-type-and-packed-selector-domains). The
combat result builders `0x0058C7D0` and `0x0058C880` pass fallback type 6 to
`0x00585800`. A spell result that keeps that fallback would not reach a held
cast begin through this arm. The writer that releases the chant when a cast
completes was not traced.

### Installed cancel entries

MEASURED. In the resident bank, `cast_stop_1`, `cast_stop_2` and `stop_mcast`
are the only schedulers that hold a RaptureCancelChantSyncClip. The four banks
of the next section hold none.

## Selected magic completion schedulers

MEASURED. These install reads cover the `mgc/base/0001` banks of
`client/chara/pc/c001/act` and `client/vfx/mgc/0101`.

| Bank | Bytes | SHA-256 |
| --- | ---: | --- |
| `cmn/mgc/base/0001` | 20960 | `4652451eab7c562659cb8ef46d7b6261ddcc45e3bba5e7d5d24bc4fb32afd558` |
| `emp_emp/mgc/base/0001` | 20512 | `403f445432662240d08fbe928cd93d2df8b31b20d925f43aede1a25edae33fbc` |
| `clw_clw/mgc/base/0001` | 27200 | `070f69f4cba4ad9af961448e68089a967741dec69d25a238b9cdefc36359ca85` |
| `client/vfx/mgc/0101` | 179344 | `4e4de9c6b9a98732e1726b2f686b3438b54f27db7d373d6057513783bdd3e57f` |

In each of the three actor banks, `main` holds BindActorClip,
RaptureCasterManagedSchClip `sht00` and RaptureActionSelectClip `mag_r1` at
frame 0, RaptureCasterManagedSchClip `mag_main` and
RaptureActionSubStatusSchKickClip at frame 30 and ClipSyncClip at frame 38.
`sht00` holds BindActorClip, RaptureClientMoveStopClip,
RaptureServerMoveStopClip and MotionClip at frame 0. In `mgc/0101`,
`mag_main` holds BindActorClip and RaptureCasterSchClip `care1_tar` at frame 0
and ClipSyncClip at frame 59. `care1_tar` holds BindActorClip,
RaptureSoundClip and ActionClip at frame 0 and
RaptureActionSelectDamageMccClip at frame 30. That bank holds no `main`.
These records do not show which action selects these banks.

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

## Monster cast scheduler sound clips

MEASURED. The pattern `client/chara/mon/*/act/emp_emp/bid/base/*` matches 77
installed files. Nested RES entries included, 58 of them hold 188 schedulers
whose names start with `cast_mon_`. None of the 188 holds a RaptureSoundClip.
The read covers this directory pattern only, and does not follow the child
schedulers that these schedulers start.

## Open

- The body of queue virtual `+0x1C`, and which bank answers a cast scheduler
  name at run time.
- What requests the `2lp` loop motions, what releases the chant on a
  completed cast and what ends a cast pose.
- The routines `0x00798720` and `0x00798780`.

## Reproduction

```text
ghidra disasm 0x00798470 -n 140
ghidra disasm 0x00798A40 -n 30
ghidra disasm 0x00798660 -n 30
ghidra disasm 0x007986C0 -n 30
ghidra disasm 0x007987F0 -n 50
ghidra disasm 0x008228A0 -n 55
ghidra disasm 0x00822BF0 -n 40
ghidra disasm 0x007CE870 -n 55
ghidra disasm 0x00845430 -n 700
ghidra x-ref to 0x0080BCD0
```

Read the three dwords at `0x012C44D4` and `0x012C4530`, the 20 string
pointers of each array, the eleven records at `0x00FE4498` and the cell
`0x00FE45A4`.
