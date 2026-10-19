# Locomotion motion bank census

The default `emp_emp` body bank of a player model holds more motion entries
than the idle clip. This page counts them: which entry names exist beside
`cbnm_id0`, their durations and key counts, which other banks carry the same
walk and run set, and what the clips do and do not encode. The
[locomotion selection finding](locomotion-selection.md) owns the run-time
choice of an entry and the state and name tables. The
[idle bank selection finding](idle-bank-selection.md) owns the path builder
and the idle kinds. The [wire move states](locomotion-wire-move-states.md) page
owns the capture census of the move state u16.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. The install's `game.ver` reads `2012.09.19.0001`
and its `ffxivgame.exe` matches the size and SHA-256.

Install counts come from Python `3` standard-library reads. A bank file is a
PWIB wrapper (signature `PWIB`, big-endian first-segment offset at `+8`) whose
first segment is a `SEDB` `RES ` directory. The directory holds a count at
`+0x30`, a name offset at `+0x34`, a name count at `+0x38`, 16-byte entries
from `+0x40` (name index, offset, length, one unread dword), and a NUL-separated
name table. Entry offsets are relative to the end of the entry array. Entries
named `RESOURCE_TYPE` and `RESOURCE_ID` are metadata and are not counted below.
Each remaining entry name is a backslash path whose last component is the
motion name, for example `cbnm_id0`. An entry is an MTB when it starts with
`SEDB` `mtb` NUL. Motion structure is read with the
[container framing](../resource/mtb-container-framing.md) and the
[motion blob](../resource/mtb-fixed-motion-calculator.md) layouts. A key frame
is `32*group + field` (bits 43 to 47 of the 48-bit key) for kinds 0 and 1 and
`255*segment + frame byte` for kinds 4 to 9. No resource bytes or hashes are
recorded here. Byte equality between entries is stated as equal or not equal.

## Bank directory shape

`client/chara/pc/c001/act` holds 995 files. 994 parse as PWIB `RES` banks and
the other one, `cmn/fid/base/2030`, starts with a bare `SEDB` `RES ` header
without the wrapper. The 994 banks hold 1828 MTB entries, every one with a
`SpuBinary` section, Header `+0x00` equal to 30.0, and 404 distinct motion
names. Name prefixes over the distinct names: `cbbm` 114, `cbem` 99, `cblm` 57,
`cbnm` 53, `cbbr` 42, `cbfm` 13, `cbmm` 9, `cbfp` 8, `cbna` 3, `cbbp` 2,
`cbnr` 2, `cbba` 1, `cber` 1.

The `emp_emp/bid/base` directory has two files.

| File | Visible entries | MTB | MCB | SCB | Other |
|---|---:|---:|---:|---:|---:|
| `0000` | 145 | 46 | 46 | 12 | 41 |
| `0001` | 47 | 16 | 16 | 0 | 15 |

The two metadata entries on top of the 145 and 47 reproduce the 147 entries
recorded for `0000` in the idle finding. Every MTB entry has an MCB entry of the
same motion name. The 12 SCB entries in `0000` are the `init*` schedulers and no
scheduler is named for a walk or run.

## The 46 clips of `emp_emp/bid/base/0000`

All 46 are version 67 containers with 17 pairs and Header `+0x00` of 30.0. The
Header `+0x04` length equals the last key frame in 44 clips. In
`cbnm_dedpose` and `cbbm_dedpose` the length is 100.0 and the last key frame is
0 (one pose, 93 quaternion keys, no quantized samples). They split into two
animation groups by the entry path: `a001\normal\miga\base` with 25 clips and
`a001\bt_emp_emp\miga\base` with 21 clips.

Normal group, lengths are Header `+0x04` frames (30 per second):

| Name | Frames | Quat keys | Quantized samples |
|---|---:|---:|---:|
| `cbnm_id0` | 203 | 3287 | 421 |
| `cbnm_01f_lp0` | 32 | 1259 | 317 |
| `cbnm_01f_l` | 35 | 1366 | 373 |
| `cbnm_01f_r` | 34 | 1352 | 416 |
| `cbnm_01f_bl` | 38 | 1553 | 449 |
| `cbnm_01f_br` | 33 | 1356 | 368 |
| `cbnm_01f_f_ls` | 41 | 1451 | 366 |
| `cbnm_01f_stp_ls` | 48 | 1271 | 296 |
| `cbnm_01f_stp_rs` | 48 | 1228 | 280 |
| `cbnm_02f_lp0` | 20 | 1053 | 418 |
| `cbnm_02f_l` | 35 | 1914 | 807 |
| `cbnm_02f_r` | 35 | 1897 | 766 |
| `cbnm_02f_bl` | 35 | 1916 | 839 |
| `cbnm_02f_br` | 35 | 1912 | 814 |
| `cbnm_02f_f_ls` | 35 | 1858 | 711 |
| `cbnm_02f_stp_ls` | 70 | 1749 | 545 |
| `cbnm_02f_stp_rs` | 70 | 1812 | 509 |
| `cbnm_trn_l` | 65 | 1242 | 264 |
| `cbnm_trn_r` | 65 | 1239 | 278 |
| `cbnm_trn_bl` | 75 | 1768 | 421 |
| `cbnm_trn_br` | 75 | 1787 | 395 |
| `cbna_add_02f_a` | 60 | 438 | 68 |
| `cbna_add_02f_b` | 60 | 556 | 100 |
| `cbnm_revive` | 160 | 3796 | 1153 |
| `cbnm_dedpose` | 100 | 93 | 0 |

No entry in this bank is named `cbnm_01f_f_rs` or `cbnm_02f_f_rs`. The
`f_ls` clips exist for both tiers and the `f_rs` partner of each is absent. The
battle group below likewise lists `cbbm_03f_f_ls` and no `f_rs`. The
banks listed in the next section repeat these clips and report no `f_rs` entry.
Absence from the other banks is not claimed, because this page does not list
every bank's entries. The client name tables carry both names
as start states 7 and 16 (see the
[selection finding](locomotion-selection.md#state-machine)). The fallback path
`0x007B9030` and `0x007B1AA0` offer for a missing name was not traced, so the
clip those two states play is open.

Battle group (`cbbm_*`, `cbbp_*`): `cbbm_id0` 100 frames, `cbbm_03f_lp0` 20,
`cbbm_03b_lp0` 28, `cbbm_03l_lp0` 30, `cbbm_03r_lp0` 30, `cbbm_03f_l`,
`_r`, `_bl`, `_br`, `_f_ls` 35 each, `cbbm_03f_stp_ls` and `_stp_rs` 70 each,
`cbbm_trn_l` and `_r` 65, `cbbm_trn_bl` and `_br` 85, `cbbm_activ` 45,
`cbbm_deact` 25, `cbbm_dedpose` 100, `cbbp_u_activ` 20, `cbbp_u_deact` 10.
The key counts of `cbbm_03f_{l,r,bl,br,f_ls,lp0}` equal those of
`cbnm_02f_{l,r,bl,br,f_ls,lp0}`. `cbnm_02f_lp0` and `cbbm_03f_lp0` are the
same size (11005 bytes) and are not byte-equal.

Name shape, INFERRED: the `01f` and `02f` tiers are two forward gaits (the
`lp0` clips are 32 and 20 frames, a cycle of about 1.07 s and 0.67 s), `l`, `r`,
`bl`, `br` are directional variants, `stp_ls` and `stp_rs` are stop clips,
`trn_*` are turn clips and `cbbm_03*` are battle-stance counterparts. The
[selection finding](locomotion-selection.md#motion-state-tables) ties these
tokens to the client's state tables: `lp0` is a loop, `stp` a stop, `trn` a turn
in place, and the `00` of the `cbbm_00*` literals is replaced by a number at run
time. The census does not test that tie against the clip data.

`0001` holds 16 clips (the idle-adjacent combat set): `cbba_add_dmg_f` 39 frames,
`cbbm_abl_1` 30, `cbbm_abl_2lp` 80, `cbbm_abl_3_can` 52, `cbbm_abls_1` 30,
`cbbm_abls_2lp` 80, `cbbm_abls_3_can` 52, `cbbm_ded` 155, `cbbm_dmg_brace` 10,
`cbbm_dmgm_{b,f,l,r}` 30 each, `cbbm_dodge_f` 80, `cbbm_pb02_dmg` 30 and
`cbbm_wekid_2lp` 100. It has no walk or run clip.

## The walk and run set outside `emp_emp`

| Where | Count | Observation |
|---|---:|---|
| `cbnm_01f_lp0` | 14 banks | 11 other `bid` banks of c001, `cmn/sid/base/0001` (32 frames), and `cmn/fid/base/0010` (186 frames, group `map_10`) |
| `cbnm_02f_lp0` | 12 banks | the same `bid` banks, 20 frames in each |
| `cbnm_01f_*` (8 clips) | `cmn/sid/base/0001` | all 8 byte-equal to the `emp_emp` entries |
| `cbnm_01f/02f/trn/id0` (21 clips) | `2ax_emp/bid/base/0000` | all 21 byte-equal to `emp_emp` |
| `cbnm_id0` | 25 banks | the `bid`, `cid` and `pid` `0000` banks and `cmn/fid/0010` |
| `cbbm_id0` | 23 banks | differs between `emp_emp` and `2ax_emp` |

`cmn/sid/base/0001` has 13 MTB entries: the eight `cbnm_01f_*` clips and five
`cbem_sit*` clips. `cmn/fid/base/0001` has 11 combat and damage clips with no
walk clip and no `cbnm_id0`. `cmn/fid/base/0010` has two clips. `cmn/rid/base/0000`
has ten mount clips (`cbnm_ch_*` and `cbnm_gb_*`: `id0`, `01f_lp0`, `02f_lp0`,
`on`, `off`), with `01f_lp0` at 38 and 40 frames and `02f_lp0` at 22 and 26.

## Other models

`emp_emp/bid/base/0000` exists for c001 through c010. Each holds 145 visible
entries and 46 MTB entries. For c001 and c002 the 46 motion names are the same
set and none of the 46 MTB entries is byte-equal. c009 has the same key and
frame counts as c001 for the clips listed above and 21 of its 46 entries are
byte-equal to c001. Frames at 30 per second:

| Model | `cbnm_id0` | `01f_lp0` | `02f_lp0` | `01f_l` | `02f_l` | `trn_l` | `01f_stp_ls` | `cbbm_id0` |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| c001 | 203 | 32 | 20 | 35 | 35 | 65 | 48 | 100 |
| c002 | 160 | 30 | 21 | 34 | 35 | 60 | 48 | 100 |
| c003 | 250 | 32 | 21 | 36 | 36 | 65 | 65 | 70 |
| c004 | 240 | 32 | 21 | 34 | 40 | 60 | 48 | 80 |
| c005 | 170 | 16 | 13 | 27 | 24 | 60 | 47 | 68 |
| c007 | 214 | 34 | 23 | 38 | 40 | 65 | 48 | 120 |
| c008 | 172 | 30 | 20 | 34 | 35 | 60 | 48 | 120 |
| c010 | 60 | 32 | 21 | 34 | 40 | 60 | 48 | 80 |

c006 matches c005 on the idle and gait lengths and differs in key counts. c021
to c025 carry fewer clips: c021, c024 and c025 hold one MTB (`cbnm_id0`) in their
`emp_emp/bid/base/0000` file, and c022 and c023 hold seven.

## What the gait clips encode

The scalar translation channels of kinds 4 to 6 on the loop clips are bounded.
In c001 `cbnm_01f_lp0` the largest value range over the cycle is 0.092 (joint
key 1, pair 1, kind 5) and the first and last sample of every channel are
equal. `cbnm_02f_lp0` gives 0.087 for the same channel and `cbnm_id0` 0.012.
No translation channel advances across the cycle, so the loop clips are in
place cycles and carry no forward travel distance. The speed at which an actor
translates is not in these banks. Constant kinds 11 to 16 and the quaternion
channels were not examined for travel.

The MCB of a walk or run clip is 800 bytes and names `PhysicsEnableDrawClip`
and `MotionCommandClip`. The stop and turn clips are 728 bytes with only
`MotionCommandClip`, as is `cbnm_id0`. No MCB string names a speed, stride or
blend input.

## Client literals

The image holds name literals for the same families. These are addresses of
strings only, read with `ghidra strings list`:

| Literals | Address range |
|---|---|
| `cbnm_01f_stp_ls`, `cbnm_01f_stp_rs` | `0x01059c18`, `0x01059c28` |
| `cbbm_00f_stp_ls` through `cbbm_00b_stp_rs` (eight), twice | `0x01059858..0x010598c8`, `0x01059a30..0x01059aa0` |
| `cbbm_activ`, `cbbm_deact`, `cbbp_u_activ`, `cbbp_u_deact`, `cbbm_hitrn_l`, `cbbm_hitrn_r` | `0x010598d8..0x0105993c`, `0x01059ab0..0x01059abc` |
| `cbnm_ch_trn_l`, `_r`, `_bl`, `_br`, `cbnm_ch_id0`, `cbnm_ch_on`, `cbnm_ch_off` | `0x01059cb8..0x01059d10`, `0x01059e40..0x01059eb0`, `0x01059fe0..0x01059fec` |
| `01f_lp0`, `01fstpl`, `cbnm_ch_01fstpr`, `02f_lp0`, `02fstpl`, `02fstpr` | `0x01059d88..0x01059e38`, `0x01059f28..0x01059fd8` |
| `cbnm_ch_02f_lp0` | `0x00fe739c` |
| `cbnm_id0`, `cbbm_id0` (many copies) | `0x00fe7270..0x00fe7580` |

Of these, `cbbm_00?_stp_*`, `cbbm_hitrn_*`, `cbnm_ch_trn_*`, `cbnm_ch_01fstpr` and
the `02fstp*` pieces have no entry of that exact name in c001 `act`. The `cbnm_`
literals `01f_stp_ls` and `01f_stp_rs` do. The `cbbm_00?` literals are templates
whose `00` the client replaces at run time, as the
[selection finding](locomotion-selection.md#motion-state-tables) records. The
composition of the other literals was not traced here.

## Open

- This page gives names and durations only. The selector, the state machine
  and the inputs they use are in the
  [selection finding](locomotion-selection.md), whose own open items remain:
  the caller of the state-machine functor and the source of its heading and
  level arguments.
- The client entries for the c2s move states 0 and 2 follow from the selection
  finding (idle and run levels). This page did not play or time them.
- The meaning of the name tokens `01f`, `02f`, `03f`, `lp0`, `bl`, `br`, `f_ls`,
  `stp`, `trn` is INFERRED from counts and lengths and from the state tables.
- The clip for the `f_rs` start states 7 and 16, which have no entry in the
  banks counted here.
- Constant-kind and quaternion channels were not checked for travel, and the
  stop and turn clips were not checked for translation travel.
- Loop wrap behavior for `lp0` against `l`, `r`, `bl`, `br` and `f_ls` clips
  (whether they are start or transition clips) is not established.
- Banks under `chara/mon` and the non-`emp_emp` `bid` banks were counted only
  for the c001 tree.
