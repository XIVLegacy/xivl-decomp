# Job-change presentation and effect banks

A retained Paladin job-activation sequence contains a job-state update
`0x01A4` and a separate presentation request `0x00DA` with selector
`0x04000029` (decimal `67108905`). The native presentation path selects
`client/vfx/lib/0041`. The job-byte setter does not itself request the effect.

The [seven-job map](#seven-job-presentation-bank-map) confirms the job-specific
banks `lib/0039` through `lib/0045` against original Lua bytecode and the
installed effect resources. Paladin also has the captured activation witness;
the other assignments are static client evidence.

The separate bank `client/vfx/lib/0048` contains the authored `jobchange`
presentation. Its effect-only selector is `0x04000030` (decimal `67108912`):
category 4 (`lib`), character bank 0, effect bank 48. That selector remains a
resource-routing result, without a captured job-change selection.

The blue-violet ribbons visible around 5:01 in RydiaMist's
[FFXIV Patch 1.21 Instance - Cutter's Cry 5 Chest Run](https://www.youtube.com/watch?v=tMN3cAYkRtI&t=301s)
are a visual reference for this investigation. Bank 48's authored name and
ring/glow resources make it a candidate for that effect. The video is labeled
patch 1.21; the binary and resources below are 1.23b. The video's exact bank
match remains unverified.

## Evidence and method

The executable is retail 1.23b `ffxivgame.exe`, build `2012.09.19.0001`,
15,996,808 bytes, image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Native addresses below are preferred VAs. Retained Ghidra listings supplied
locators. Each listed instruction in `0x00798470`, `0x007982D0`,
`0x00798640`, `0x00798BF0`, `0x00799C90` and `0x00706DC0` was compared with
file-backed PE bytes and decoded individually using pefile 2024.8.26 and
Capstone 5.0.7 in x86-32 mode. Category records, vtable cells and path strings
were read from the same image.

Bank 48 is 181,056 bytes, SHA-256
`c986f5d5c3f65d8d1eb481515300a301df01db7aa5a1e4b583095b85f6946b19`.
Python standard-library reads and the repository's `tools/vfx_qix_resource.py`,
`tools/vfx_effect_body.py` and `tools/scb_clip_scheduler.py` decoded its
PWIB/RES directory, Qix roots and SCB records. Every Qix blob passed the
existing relocation-directory checks. Resource offsets below are absolute
file offsets, not offsets into an extracted payload.

## Bank 48 selector and native resource request

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

## Captured presentation trigger

The retained source is `gear_changesoul.pcapng`, 23,784 bytes, SHA-256
`62af2cde3a4683fba3e09cc3dfed8001d74303b120381de577b05df492926780`.
Source identity belongs to `xivl-captures:sources/pcap-1.23b/manifest.yaml`.
The existing `xivl-captures:studies/map-00da-00e1-comparison/derived/occurrences.csv`
row for this capture records s2c frame 14, subevent 0, stream offset 1182,
with first application dword `0x04000029`. The second application dword is 0.
Its `derived/neighborhoods.csv`, anchor 2, relative event -1, records the
same-actor `0x01A4` at frame 13, subevent 0, stream offset 1128.

`xivl-captures:studies/c2s-012d-event-start/derived/job-change-context.json`
adds the job byte `16` and the same-connection request context. The
[Equipment job table](../script/equipment-soulstones.md#important-item-backing)
identifies job 16 as Paladin. The c2s `0x012D` request is frame 19,
subevent 0, stream offset 3416. Its event name is `commandRequest`, its
static-owner low 16 bits are 12017, and its typed tail begins with integer
1 followed by eight nils. The request and job update have equal actor fields.
The [request-side mapping](job-soulstone-state.md#text-command-and-macro-requests)
submits command 12017 with mode 1 for job activation.

The s2c presentation request is the next wrapped-actor message after that
job update in the same connection and direction. This is wire order, not
proof of native execution order or rendering. The separate c2s request
cannot be ordered against those s2c frames by comparing their frame indices.

The same context product checked 65,864 complete decoded subevents across
the 54 retained source members. It found neither exact `0x04000030` nor a
packed selector satisfying `(value & 0xFF000FFF) == 0x04000030`, which also
allows a nonzero character-bank field. This is a bounded absence in admitted
complete subevents, not a rule excluding bank 48 from retail playback.

The [action-queue consumer](action-queue.md#boundary-with-wire-and-vfx-systems)
owns this `0x00DA` route:

```text
0x0058CCA0 case 0x00DA at 0x0058CFFA
  -> 0x0058CAD0 -> 0x0058C690
  -> 0x00585800(5, 0x04000029): visual class 12
  -> 0x00587370 -> 0x00587210: record+0x10 = selector
  -> 0x005901D0: CharaElement+0xA80 ring
  -> 0x0058DA10: class-12 bucket, Scene command 4
  -> 0x00662D30 case 4 -> 0x00846080 -> 0x00845E80
  -> CharaActionQue -> CharaActionVisual -> 0x00798470
  -> 0x00799C90: /client/vfx/lib/0041.bin
```

At `0x0058CFFA`, the dispatcher reads the first application dword, view
`+0x10`, and passes its value to `0x0058CAD0`. That helper obtains the current
element's actor ID through `0x004D6750` and supplies it as the target to
`0x0058C690`, with route-control value 0. The builder also uses that resolved
actor as its source. The second on-wire dword is not read on this path.

The classifier splits `0x04000029` into category 4, middle 0 and low 41.
Category 4 has route kind 1, and middle 0 misses the special middle-bank
range 13 through 19 and value `0xFE9`. The branch at
`0x005858AB-0x005858D6` therefore returns class 12. The
[drain](../net/combat-result-element.md#drain) admits that class without
the classes-3-through-11 header-call path. This is presentation routing,
not a job-change success test.

The [concrete visual dispatch](action-queue.md#concrete-visual-dispatch)
preserves the staged selector at visual `+0x3C` and resolves visual slot
`+0x40` to `0x00798470`. Source and target actor fields remain separate;
the receiving element is selected by the sub-event header source. Element
readiness, actor binding, queue admission and resource availability still
apply.

The [combat-result consumer](../net/combat-result-element.md#opcode-routes)
also accepts packed selectors, at a different application offset. That
alternative route does not establish a combat-result trigger for this
captured job presentation.

Original-PE instruction comparison additionally covered the receiver
`0x0089CB60`, presentation helpers `0x0058CAD0` and `0x0058C690`, result
wrappers `0x0058C880` and `0x0058C7D0`, stagers
`0x00587210` and `0x00587370`, classifier `0x00585800`, selector splitter
`0x00798370`, drain `0x0058DA10`, insertion `0x00846080` and `0x00845E80`,
and the visual constructors and dispatch at `0x007983C0`, `0x00843DB0`,
`0x00843B50` and `0x00798AA0`. The complete dispatcher range
`0x0058CFFA-0x0058D004` was also compared and decoded from original PE bytes.
The existing `tools/verify_combat_result_element.py --exe` and
`tools/verify_player_profile_accessors.py --exe` checks also passed against
the pinned image. These checks observed static bytes, not execution.

## Seven-job presentation bank map

The [Equipment job table](../script/equipment-soulstones.md#important-item-backing)
owns the job IDs and soul-stone item identities. The retail helper
`QuestBaseClass.showGetJobItemWidget`, decoded at
`xivl-client-scripts:lua/scripts/quest/questbaseclass_common.lua:1189-1228`,
assigns these selectors for those exact items and calls
`playerArgument:_runCharaScheduler(selector)`. This is an explicit script
assignment, not a name guessed from the bank filename.

| Job | Job ID | Soul-stone item | Bank | Packed selector | Original EQ / LOADK PCs |
| --- | ---: | ---: | --- | --- | --- |
| Warrior | 17 | 2000203 | `lib/0039` | `0x04000027` | 42 / 44 |
| Monk | 15 | 2000202 | `lib/0040` | `0x04000028` | 46 / 48 |
| Paladin | 16 | 2000201 | `lib/0041` | `0x04000029` | 50 / 52 |
| White Mage | 27 | 2000206 | `lib/0042` | `0x0400002A` | 54 / 56 |
| Black Mage | 26 | 2000207 | `lib/0043` | `0x0400002B` | 58 / 60 |
| Dragoon | 19 | 2000204 | `lib/0044` | `0x0400002C` | 62 / 64 |
| Bard | 18 | 2000205 | `lib/0045` | `0x0400002D` | 66 / 68 |

Each selector has category 4, middle field 0 and its bank number in the low
12 bits. The native route already traced above therefore selects its
corresponding `/client/vfx/lib/NNNN.bin` request. Every mapped selector is
visual class 12. The job ID, item ID and bank number are distinct namespaces.

### Original script verification

The decoded helper is 35,511 bytes, SHA-256
`cf61a5687dbc7b6accd26c8cd3a138decc0372fb72518ac6f2054f125bc1465e`,
authenticated against `xivl-client-scripts:manifests/scripts.json`.
The original `client/script/tp5rq/tp5rq89r57y9rr_7vxxvw.le.lpb` is
13,072 bytes, SHA-256
`ecc3f7c6fb49df196431494aa5aece95ede1c24ca1325af11ddc993a3836322d`.
Its decoded Lua payload is 13,059 bytes, SHA-256
`9379ee6832bdfa542273c7f43f065d7a15f9a730d9ef244be9a565a1faa53504`.
Those original and payload identities match
`xivl-client-scripts:manifests/retail_lua_coverage.json`.

`xivl-client-scripts:tools/retail_script.py:decode_lpb` decoded the original.
`tools/build_quest_contracts.py:ChunkReader` in that repository parsed the
payload through EOF. In `root/proto40`, each tabled `EQ` compares parameter
register 2 with its item constant. The equality branch reaches the listed
`LOADK` into register 7; its following jump reaches PC 115. PCs 115-117 use
`SELF` on parameter register 1 with `_runCharaScheduler`, move register 7
into its argument slot and call with the receiver and selected ID. The
bytecode therefore confirms the item-to-selector branches and forwarding,
independently of the recovered Lua rendering.

### Component effect resources

All seven banks contain two `veff` blobs. These are the authored source
basenames, containing directory names and absolute blob offsets within the
extensionless bank file. The common authored directory prefix is
`d:\gra_rapture\vfx\lib\`; each directory below has a `vfx` child.
The two lifetime values are the respective veff root `+0x50` tick words.
They are not the SCB action-end fields.

| Job | Authored directory | First veff | Second veff | Blob offsets | Lifetime ticks |
| --- | --- | --- | --- | --- | --- |
| Warrior | `job_stn39` | `job_stn39.veff` | `s_weapon.veff` | `0x110A0`, `0x189E0` | 600000, 375000 |
| Monk | `job_stn40_mnk` | `jobstn40_mk.veff` | `s_weapon.veff` | `0x110B0`, `0x18A60` | 550000, 375000 |
| Paladin | `job_stn41_pld` | `job_stn41_knight.veff` | `s_weapon.veff` | `0x10C00`, `0x18670` | 550000, 375000 |
| White Mage | `job_stn42_whm` | `job_stn42_wm.veff` | `s_siroma.veff` | `0x1D1E0`, `0x24CC0` | 550000, 350000 |
| Black Mage | `job_stn43_bkm` | `job_stn43bm.veff` | `s_kuroma.veff` | `0x18A70`, `0x203C0` | 600000, 350000 |
| Dragoon | `job_stn44_drg` | `job_stn44_dk.veff` | `s_weapon.veff` | `0x10C00`, `0x18E40` | 600000, 375000 |
| Bard | `job_stn45_brd` | `job_stn45_gn.veff` | `s_song.veff` | `0x13860`, `0x1B1B0` | 600000, 600000 |

The same repository RES, Qix-root and SCB parsers used for the Paladin bank
decoded every bank above. All Qix relocation-directory checks passed.
The resource contents corroborate the script assignments, including Warrior's
otherwise generic `job_stn39` name. They do not prove rendered colours or a
universal server policy for job-change playback.

### Authored action and sound timing

Each SCB binds one proxy actor, starts action 1 and sound ID `2346713177`
at tick 0, then starts another action and sound at tick 400000. Its first
ActionClip ends at tick 550000. The later action differs as follows:

| Job | Later action ID | Later action end tick | Later sound ID |
| --- | ---: | ---: | ---: |
| Warrior | 2 | 780000 | 2346713179 |
| Monk | 3 | 780000 | 2346713182 |
| Paladin | 2 | 780000 | 2346713183 |
| White Mage | 2 | 750000 | 2346713184 |
| Black Mage | 2 | 750000 | 2346713181 |
| Dragoon | 2 | 780000 | 2346713180 |
| Bard | 2 | 1000000 | 2346713185 |

These are serialized ActionClip IDs, timing fields and sound resource IDs.
In particular, Monk uses action 3 for the later entry. The numeric action-to-ACB
lookup and final Qix playback are not established by this resource census.

### Resource identities

The matching installed build was `2012.09.19.0001`. The bank paths are relative
to `client/vfx/`; the [Paladin identity](#captured-paladin-bank) is recorded
with its capture-backed resource join.

| Bank | Bytes | SHA-256 |
| --- | ---: | --- |
| `lib/0039` | 221392 | `96fcada7d23163cc701a67422bf4fefe4334f7c2a0ffe907dfe927f49c7af54a` |
| `lib/0040` | 221584 | `affed652927251d5cd3aa7312c3a667c79ddc0a57516e365338be9eb2d686db1` |
| `lib/0042` | 321360 | `a2b4b91628ddd0d5287a4acf0c6bea41d56a414a1be2240ec97fcbd5930e3dcb` |
| `lib/0043` | 285360 | `b1d26c2436fe57b8ac40481290d72eede2dc5d30ddc6c0a90245547b9d31f2a7` |
| `lib/0044` | 223760 | `a666986bfc5b6dca3d114bdd291479882a4995dc25504f291448e7e49b05721b` |
| `lib/0045` | 296272 | `e51b4c1d72880bfdd367436d533f6e65f04d556137c8a83ef52c8a7c7504cf18` |

The script helper presents soul-stone rewards, so its assignment proves a
job-specific presentation family. The captured Paladin activation independently
uses the same selector. Equivalent activation captures for the other six jobs
are not established here. Bank 48 is separate from all seven explicit
soul-stone branches and remains unassigned to an individual job.

## Captured Paladin bank

`client/vfx/lib/0041` is 221,760 bytes, SHA-256
`83f85907f74b57a90918558ffe1c089e4a8a099e7d3c7a5643ca06627ef5a519`.
The same RES, SCB and Qix parsers used for bank 48 recover its `j41pld`
resource family, `main` scheduler, `jobstn_pld` and `jbstn41_2` action names.
The veff roots include these authored paths:

- `d:\gra_rapture\vfx\lib\job_stn41_pld\vfx\job_stn41_knight.veff`
- `d:\gra_rapture\vfx\lib\job_stn41_pld\vfx\s_weapon.veff`

The SCB has an action starting at tick 0 and another at tick 400000, each
with a sound trigger at its start. Its final ActionClip ends at tick 780000,
about 2.6 seconds under the scheduler clock interpretation. These authored
records do not by themselves prove rendered playback.

The [job-state finding](job-soulstone-state.md#actions-and-acquisition-boundaries)
also records use of this selector by the quest item-presentation helper.
Resource identity therefore does not distinguish soul-stone acquisition from
job activation; the captured request and state context supply that distinction.

## Bank 48 scheduler, effect and sound

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

The captured presentation has an explicit `0x00DA` selector. It supplies a
concrete delivery mechanism for the Paladin witness, without making every
job-byte write play an effect. The same-actor selection of bank 48 and the
video's exact bank match remain unverified.
