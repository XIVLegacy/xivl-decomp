# Job-change effect bank

The retail 1.23b effect bank `client/vfx/lib/0048` contains the authored
`jobchange` presentation. Its packed effect-only selector is `0x04000030`
(decimal `67108912`): category 4 (`lib`), character bank 0, effect bank 48.
This is a resource-routing result, not a captured job-change packet or a
recorded playback test.

The blue-violet ribbons visible around 5:01 in RydiaMist's
[FFXIV Patch 1.21 Instance - Cutter's Cry 5 Chest Run](https://www.youtube.com/watch?v=tMN3cAYkRtI&t=301s)
are a visual reference for this investigation. The bank's authored name and
ring/glow resources make it a strong candidate for that effect. The video is
labeled patch 1.21; the binary and resource below are 1.23b. Their exact visual
equivalence remains unverified.

## Evidence and method

The executable is retail 1.23b `ffxivgame.exe`, build `2012.09.19.0001`,
15,996,808 bytes, image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Native addresses below are preferred VAs. Retained Ghidra listings supplied
locators. Each listed instruction in `0x00798470`, `0x007982D0`,
`0x00798640`, `0x00798BF0`, `0x00799C90` and `0x00706DC0` was compared with file-backed
PE bytes and decoded individually using pefile 2024.8.26 and Capstone 5.0.7
in x86-32 mode. Category records, vtable cells and path strings were read
from the same image.

The installed bank is 181,056 bytes, SHA-256
`c986f5d5c3f65d8d1eb481515300a301df01db7aa5a1e4b583095b85f6946b19`.
Python standard-library reads and the repository's `tools/vfx_qix_resource.py`,
`tools/vfx_effect_body.py` and `tools/scb_clip_scheduler.py` decoded its
PWIB/RES directory, Qix roots and SCB records. Every Qix blob passed the
existing relocation-directory checks. Resource offsets below are absolute
file offsets, not offsets into an extracted payload.

## Selector and native resource request

The [bank-routing finding](animation-bank-routing.md) owns the general packed
selector representation. For this bank, the checked native path is:

| Locator | Observation |
| --- | --- |
| `0x00798478`, `0x00798480`, `0x00798510-0x00798523` | Read the packed selector from visual `+0x3C`; split its category, middle 12 bits and low 12 bits. |
| `0x007982D0`, category record `0x00FE3308` | Category 4 has name `lib`, character/effect flags 1/1 and route kind 1. |
| `0x007984E2-0x007984F0` | Pass category, low bank and route kind through visual vtable `+0x58`. |
| Vtable cell `0x00FE459C` | The concrete effect loader is `0x00799C90`. |
| `0x00798C6F-0x00798C77` | The character loader `0x00798BF0` rejects a nonpositive character-bank number. |
| `0x00799CD2-0x00799CE5` | Reject nonpositive category or effect-bank values. |
| `0x00799D94-0x00799E13`, `0x00798640` | For route kind 1, use virtual `+0x5C` to obtain the category name from the category table. |
| `0x00799EA4-0x00799EB4` | Format the effect bank with `%04d`. |

Strings at `0x00FE3C00`, `0x00FE3C18`, `0x00FE3C1C` and `0x00FE3C24`
are `/client/vfx/`, `/`, `%04d` and `.bin`. Category 4 and effect bank 48
therefore select the logical request `/client/vfx/lib/0048.bin`, corresponding
to the installed extensionless bank. The zero middle field skips the character
bank; it is not a job ID.

No literal little-endian `0x04000030` occurs in the pinned executable.
The selector is derived from the loader and resource identity, rather than
recovered from a native job-change callsite. The
[action queue](action-queue.md#boundary-with-wire-and-vfx-systems) documents
generic animation delivery through s2c `0x00DA`, `0x00E0` and `0x00E1`.
That generic reachability does not identify which message selected this bank
during a retail job change.

## Authored scheduler, effect and sound

The outer RES starts at file offset `0x10`. Its name table pairs entry 0
with `main\bin\main` and entry 1 with `lst\sch_effect_data\jobchange`.
Entry 0 is an SCB at `0x90`, size `0x430`; entry 1 is a nested RES at
`0x4C0`. The SCB declares one `ProxyActor` and these clips, all on actor
ordinal 0 and starting at tick 0:

| File offset | Clip | Selected fields |
| ---: | --- | --- |
| `0x2E0` | `BindActorClip` | Class-key byte 2; record size `0x14`. |
| `0x2F4` | `RaptureSoundClip` | Class-key byte 3; flags `D+0x14 = 1`, sound slot `D+0x18 = 0`, volume `D+0x1C = 1.0`, one resource ID at `D+0x28`. |
| `0x320` | `ActionClip` | Class-key byte 1; action ID `D+0x10 = 1`, end tick `D+0x18 = 900000`, threshold `D+0x20 = 900000`. |

The ActionClip's authored end is three seconds under the
[scheduler tick conversion](../resource/cut-scheduler-clip-timing.md#time-unit).
The sound resource word is decimal `2346713178`. The class-key byte 3 array
path selects its sole element; the
[RaptureSoundClip consumer](../resource/cut-scheduler-clip-payloads-ranks-14-22.md#rapturesoundclip)
owns the native playback conditions. The authored trigger does not prove
audible playback or identify a decoded sound file.

The nested RES contains these effect roots:

| File offset | Type | Root name |
| ---: | --- | --- |
| `0x4100` | `veff` | `1iCy3Kjob_cheng` |
| `0xDB30` | `leaf` | `1Yhajujob_chang` |
| `0xDFF0` | `vins` | `1jSCSnvleafinst` |
| `0xE480` | `ACB` | Directory name `main\bin\jobchange`. |

The veff's root record begins at `0x5600`. Its source-path field resolves to
the authored string `d:\gra_rapture\vfx\lib\job_change\job_chenge.veff`.
The spelling `chenge` is preserved from the resource. Root `+0x50` holds
`880000` lifetime ticks, about 2.93 seconds using the
[effect clock interpretation](../resource/vfx-effect-runtime.md#time-and-the-clock-constants).
The bank also contains model roots `423nCxglo4_u4_0` and `1qYLUkd_ring`
and texture roots including `3bX0Fkglo_4_0`, `34HcHZdist` and
`15At1uns_ring`. These names support the ring/glow candidate identification;
they do not establish its rendered colour, geometry, or camera appearance.

The directory identity and typed roots are established. The ActionClip's
numeric ID has not been traced through its runtime action lookup to the ACB
and Qix instance, so that final playback join remains conditional.

## Job state and playback boundary

[Active job and soul-stone state](job-soulstone-state.md#active-state-and-notification)
owns the changed job-byte route: receiver `0x0089CB60` calls setter
`0x00706DC0`, which prepares `_onChangeJob(newJob, oldJob)` after a changed
value. The recovered Lua callback has an empty body. The setter contains
no direct request for bank 48. Updating the job byte alone therefore does
not establish playback of this effect.

A same-actor capture joining job/equipment updates to the presentation
selector, or a recorded client resource/playback trace, is still needed to
prove the job-change delivery sequence and the video's exact bank match.
