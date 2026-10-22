# Stock seasonal component bindings

The original 1.23b DATs retained Halloween and Starlight assets, resident
decoration owners and scheduler controls. Whole-file absence of an earlier
weather layout did not prove absence of its component assets. The retained
controls provide stock-only composition candidates; they do not yet establish
a complete, persistent switch between both seasonal presentations.

This finding separates three questions: which components survived, which
layouts composed them, and which native inputs could select those layouts or
their controls. It records offline evidence only.

## Inputs and producing tools

All native addresses are VAs in retail `ffxivgame.exe`, build
`2012.09.19.0001`, image base `0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Retained Ghidra 12.1.3 listings supplied locators; pefile `2024.8.26` and
Capstone `5.0.7` independently checked original bytes and x86-32 instructions.
These were observations of code, not calls executed in a client.

The original `RegionResourceData`, `data/03/C0/00/00.DAT`, was 52,336 bytes,
SHA-256 `c04b0d998aea4c1b13ed322292a5aa5af45485c698da2315171c3c024bcb9a74`.
Python `struct` followed its declared root/child runs. Typed layout joins used
`xivl-tools:tools/research/census_layout.py`, `parse_layout`, revision
`75fe7414bf3f1ecd779825083d477adc4ed5f319`, script SHA-256
`5afccbd58e7faa491f860ce51e59a8c65a4a9d1850931e38f577d2fc3390e5e3`.
The [public Halloween owners](city-halloween-owner-joins.md),
[interior owners](city-interior-halloween-owner-joins.md#original-resources-and-method)
and [Limsa public owner](limsa-seasonal-weather-extension.md) pin the six
original layout DAT identities and explain the reference framing.

## Surviving leaves and composed atmosphere

The [weather catalog](city-weather-catalog.csv) pins the final city weather
DATs and registry rows. Its Gridania 8027 / `0x29B00020` was 118,768 bytes,
SHA-256 `44cff7e75f20f8c0735f4ecd91bc1e7c2b9c28551806d5ee3fa3e5b1b1a122f1`.
Its two VFX nodes had explicit named external inputs:

| File locator | Typed observation |
| --- | --- |
| `0x6460`, Instance `cbind_xmas`, `+0x3C` | Referenced VFX `vfx_cam_xmas`, payload-relative `0x8528`. |
| `0x8A30`, VFX `vfx_cloud` | String fields `+0x18/+0x1C/+0x20` named `0yT9Qtvleafinst`, `24Kfgpxmas_clou`, `1jKnQTw32xmas1s`. |
| `0x8A58`, VFX `vfx_cam_xmas` | Those fields named `0yT9Qtvleafinst`, `1xeW2Rcam_xmas`, `0uA4Ztxmas0001s`. |
| Outer rows `0x3E0` and `0x4E0` | Both named controller `0yT9Qtvleafinst`, target `0x5D200019`. |

That controller was a 1,632-byte `SEDBvins`, SHA-256
`bfbe1271972f666e786e64e3cc1a9fb63f872c64bfd3a1b3533d7ef93f91cfd8`.
Its strings at `0x344/0x394` named `24Kfgpxmas_clou`; those at
`0x3E4/0x434` named `1xeW2Rcam_xmas`. Python `struct` followed the layout
references and outer resource rows; bounded string reads and `hashlib`
checked this controller. These joins establish current effect inputs,
without treating a marker as proof of rendered snowfall.

The [recovered October Gridania layout](gridania-halloween-weather-restoration.md#layout-and-retained-dependencies)
had both Halloween-specific external leaves still present byte-for-byte in
stock: `0x72E8006F` (`SEDBtxb`) and `0x5B13008C` (`SEDBSSCF`). It also
contained embedded DrawEnv, Fog, light, environment and time-curve records.
For example, original `dwev_00` at file `0x63B4` referenced Fog,
light-set, environment-map and screen-environment records through
`+0x20/+0x24/+0x28/+0x30`. Existing leaf files alone did not select that
embedded composition. The October wrapper was a separately recovered patch
input, not another stock file authenticated by this component graph.

### Embedded records that survived

Python `struct` compared original October and final Gridania nodes by
declared class and name. Removing declared class/name/string reference
fields before hashing retained the other inline bytes of each LightBase
record. Separate comparisons followed Fog `+0x10` to a bounded `0x30`-byte
body and TimeLine `+0x14` to each declared complete `SEDBSCB` extent.

| Compared role | Original October / final file locators | Observation |
| --- | --- | --- |
| Fog bodies | `fog_default` bodies `0x66B0` / `0x6A60`; `fog_sky` `0x66E0` / `0x6A90` | All 11 named Fog bodies matched by role. These examples shared SHA-256 `7cbb6dbbdf9b36fd0f451e6207506f1a9722c539582c287df24e73dbdf01a25e`. |
| Inline light records | `main_lght_sky` `0x6F80` / `0x7320`; `amb_lght_cld` `0x70C0` / `0x7460` | All 29 named 80-byte LightBase records matched after the stated reference normalization. Their example normalized hashes were `1ec30c0a6432f2d8827bb783b00ab07cadb4771bfd496a77d2f98e0165bb5ed5` and `7589b68895df2222d7edb17ce815a6b5dd9e1895d7490e4d7151d15d260271ce`. |
| Embedded `time_dwev_11` SCB | `0x17450` / `0x14B10`, 4,320 bytes | Exact body match, SHA-256 `9237be638e37fa36c8287c71f8b968bc9c17a74e8bf1ab487deb04f2fbc4f906`. |
| Embedded `time_dwev_12` SCB | `0x19950` / `0x17010`, 4,320 bytes | Exact body match, SHA-256 `6387d21111711d5c165f028ea17eb45f24fd7bf7d8601dac958a1c85ce68c6c3`. |
| Embedded `time_dwev_22` SCB | `0x1AA30` / `0x180F0`, 4,320 bytes | Exact body match, SHA-256 `9e5e5c3c21716dbdeb88937cbb434cda2f34b4d92d5ada1898a7a44e7e8795fc`. |

Thus useful October environment content survived inside the final wrapper,
in addition to its external leaves. Of the final wrapper's 12 nonzero SCB
bodies, these three matched the original whole bodies. Differences in the
other bodies did not by themselves prove different animated colours: clip
metadata, targets and curves require separate interpretation. The typed
LightSet and environment-map joins below distinguish relocated pointers
from different resource inputs. This comparison establishes partial content
retention, without claiming complete Halloween equivalence or impossibility.

### Clear-weather timeline and curve reuse

Stock Gridania clear weather `0x29B0000F` was 129,856 bytes, SHA-256
`4c2627bd34f53d445d0834677e5064fcecaffd0d13b07dae8fbb698b323a98a6`.
Its declared TimeLine spans contained seven of the twelve complete October
SCB bodies byte-for-byte: the three `11/12/22` bodies above and these four:

| TimeLine | October / stock clear SCB file offset | Bytes / shared SHA-256 |
| --- | --- | --- |
| `time_dwev_cld` | `0xEEC0` / `0xE5F0` | 3,296 / `d3950c04505ed07805cadd605524c2c75bf82e30137b43b9fb8e507d5a30b2a4` |
| `time_dwev_10` | `0x14050` / `0x136F0` | 5,968 / `4087f335158428bc23650f425a0d0cd511c0f696ca5dbc785096f3de53da1c33` |
| `time_dwev_20` | `0x157A0` / `0x14E40` | 7,344 / `35f3f4c5920feaa5ce76743153ffc8816034b319d1c0565c31526ca9f92bffc3` |
| `time_dwev_21` | `0x18530` / `0x17BD0` | 5,152 / `561012c4686e4e7ec68b6150997936d37f7b2df97bb9fb30620bb95b6d03ced9` |

Python `struct` also followed each declared SCB root directory to its
`@FCURVE` resource chunk. Its count/16-byte-unit entry offsets bounded each
descriptor; descriptor `+0x10` supplied a length, and the MTB body began at
descriptor `+0x20`. Every compared body fitted its entry and began with
`SEDBmtb`. This length was separate from the MTB header's zero size word.
All 47 present weather files in the catalog matched their recorded lengths
and SHA-256 digests before this comparison.

Of the 63 October MTB entries, 46 had exact body matches among those files.
The other 17 belonged to `time_dwev_sky`, `time_dwev_00`, `time_dwev_01`
and `time_sqnc_24`. For example, October `time_dwev_00` entry 1 at file
`0x10BA0` had 2,404 bytes and SHA-256
`f7e0a77d52384e9f30a68987935b045c2dfbef142ec8743dade26c188da1c206`;
no compared city-weather MTB body matched it. This finite byte-identity
comparison does not exclude differently encoded equivalent curves or
establish rendered colour differences. Reusing clear weather therefore
recovers identified whole timelines, without establishing a complete
October Halloween atmosphere.

A broader Python `os.scandir`/`hashlib` scan read 140,180 regular files,
7,444,438,570 bytes, under the original `data` tree without following
reparse points; no entries were skipped. It examined 10,998 raw
`SEDBmtb\0` positions, selected candidates by the targets' first 128 bytes,
then compared each complete declared target body by length and SHA-256.
None of the 17 remaining October bodies matched. SHA-256 over sorted
ASCII `relative/path`, NUL, decimal byte length, NUL, file SHA-256 and LF
was `76771ffe86e4f76a05d214131324936829119e6ee37724c19188f86a568ae2bb`.
This identifies the finite bytes read; it does not exclude equivalent or
compressed encodings and is not a client playback result. A stock-file
substitution alone therefore has no authenticated exact source for those
17 October curve bodies.

### LightSet members and environment-map references

The declared LightSet `+0x10/+0x14` pointer/count pairs led to member
records, their InstanceObject inputs and ordinal-selected UnitTree children.
Resolving those children to typed LightBase names produced identical ordered
members in all ten October/final LightSets. The nine cloud/weather lists
contained `main_lght_*`, `sub_lght_*`, `amb_lght_*` in that order; the sky
list contained `main_lght_sky`, `sub_lght_sky`. Different serialized addresses
did not represent different membership.

| LightSet | October node / pointer-array file offset | Final node / pointer-array file offset | Resolved members |
| --- | --- | --- | --- |
| `ltst_00` | `0x78C8` / `0x79BC` | `0x7C68` / `0x7D5C` | `main_lght_00`, `sub_lght_00`, `amb_lght_00` |
| `ltst_01` | `0x798C` / `0x7A10` | `0x7D2C` / `0x7DB0` | `main_lght_01`, `sub_lght_01`, `amb_lght_01` |
| `ltst_sky` | `0x7890` / `0x79A8` | `0x7C30` / `0x7D48` | `main_lght_sky`, `sub_lght_sky` |

EnvironmentMapDrawEnvObject `+0x14` instead pointed to a declared eight-byte
record holding two string pointers. Resolving the leaf string against the
outer resource rows gave these examples, with all offsets relative to files:

| Role | October node / nested record / outer row -> leaf | Final node / nested record / outer row -> leaf |
| --- | --- | --- |
| Common `envmap_00` | `0x6D5C` / `0x6F00` / `0xE0` -> `39Vlyxenv_00_de`, `0x615F0000` | `0x7120` / `0x72A8` / `0xE0` -> the same string and key |
| Seasonal 000 | `f0f_fine_000`, `0x6E20` / `0x6F38` / `0x1A0` -> `2TiVnFf0f_hall_`, `0x72E8006F` | `f0f_mist_000`, `0x71E4` / `0x72E0` / `0x1A0` -> `34BjQMf0f_rain_`, `0x72E80331` |
| Seasonal 120 | `f0f_fine_120`, `0x6E74` / `0x6F50` / `0x200` -> `2cFxalf0f_fine_`, `0x72E80312` | `f0f_mist_120`, `0x7238` / `0x72F8` / `0x200` -> `2mKcmxf0f_mist_`, `0x72E8031B` |

The remaining shared `envmap_01/10/11/12/20/21/22` roles also resolved
to identical leaf strings and keys. The seasonal leaves differed, including
the 050/060/180/190 roles and October-only `f0f_h074`. The names above
identify resource inputs; they do not authenticate the rendered colours.
Python `struct`, the pinned typed parser and original file hashes established
these joins. Together with the matched Fog/LightBase data, they locate
specific remaining differences in seasonal environment-map inputs and
nonmatching SCB bodies. Their native activation/use remains a separate
compatibility requirement; this is not an exhaustive equivalence check.

### Cross-city and Starlight compositions

Limsa 8027 / `0x29D9001F` and Ul'dah 8027 / `0x615A0023` retained their
original October bytes, as the
[patch comparison](gridania-halloween-weather-restoration.md#original-patch-and-extraction)
established. Each was a registered weather layout with DrawEnv/Fog lanes
and a Halloween SE reference:

| Weather layout | Original file locator | Halloween sound input |
| --- | --- | --- |
| Limsa `0x29D9001F` | SE `sdef_0013` at `0x83C0`, field `+0x14` | `2yMuMMcoasthall`; outer row `0x440` selected `0x8A2600B1`. |
| Ul'dah `0x615A0023` | SE `sdef_0013` at `0x6CD0`, field `+0x14` | `2mWb4qwildshall`; outer row `0x400` selected `0x89820090`. |

Reusing either layout in Gridania is a stock-file candidate. Their local
DrawEnv, instance, sound and texture bindings differed. Native key selection
is established below; a correctly resolved/rendered result remains unproven.

The [exact-name DrawEnv consumer](weather-sky-fog-environment-path.md#lane-selection-by-spatial-drawenv-name)
replaced a selected zone DrawEnv only when its name existed in the weather
work set. Otherwise the selected zone object remained. Both cross-city
8027 wrappers contained the common named weather lanes but no `dwev_01`.
The Gridania public town and field layouts did contain that lane:

| Original Gridania DAT | Bytes / SHA-256 | Typed `dwev_01` file locator |
| --- | --- | --- |
| `data/29/B0/00/01.DAT`, town | 1,300,928 / `abc13bd6f096b0fe9703b2092ca4a7f819ff2b88e4d08ae222a9db447f618312` | `0xE9CB4`, referencing `fog_0001` and `ltst_0001` |
| `data/29/B0/00/03.DAT`, field | 1,869,728 / `77d15ce02d595a56ddea8ee2103e2a9ac075b7574fa302e81d9087f2742b67ba` | `0x14D1C4`, referencing `fog_0003` and `ltst_0003` |

Python's typed parse of these complete files therefore joined a concrete
missing weather-name case: if a position selected either zone `dwev_01`,
neither cross-city wrapper could replace it by that name. The
[zone-lighting interpretation](zone-lighting-source-and-time-of-day.md#zone-layout-lights)
keeps the subsequent rendered LightSet use qualified. Gridania interior
`0x29B00002` had five named DrawEnv objects and no `dwev_01`; the same
missing-lane claim does not apply there. Cross-city key admission alone
therefore did not establish a complete Gridania atmosphere replacement.

The [Starlight residue graph](city-seasonal-weather-selectors.md#weather-payload-reuse)
already identifies surviving components, including leaves without a current
layout owner. The final 8032 resources had last-weather/thunder bindings;
their legacy `wtr_xmas` tokens did not make them intact alternate Starlight
compositions. No complete alternate historical Starlight bank was established
by the graph. This is a bounded composition result, not proof that its
individual effects cannot be reused.

## Resident decoration controls

The [seasonal selector census](city-seasonal-weather-selectors.md#six-city-layouts)
and typed owner joins authenticated 60 Halloween pairs and five interior
Starlight pairs. The Halloween show vectors selected 8027 alone; their hides
selected its complement. All five final Starlight show vectors selected no
authored ID, while their hides selected every declared ID. Their earlier
8032 binding did not survive as an active final-client switch.

Following each UnitTree's declared child records and reverse instance
references produced this request-alias split:

| Layout selector / resource key | Halloween owners with `show`/`hide` aliases | Halloween owners with longer aliases | Starlight owners with `show`/`hide` aliases |
| --- | ---: | ---: | ---: |
| 321 / `0x29B00001`, Gridania public | 11 | 0 | 0 |
| 331 / `0x29B00002`, Gridania interior | 0 | 10 | 1 |
| 121 / `0x29D90001`, Limsa public | 8 | 0 | 0 |
| 131 / `0x29D90002`, Limsa interior | 14 | 0 | 2 |
| 421 / `0x615A0001`, Ul'dah public | 10 | 0 | 0 |
| 431 / `0x615A0003`, Ul'dah interior | 7 | 0 | 2 |

These selectors were registry child keys, not region keys 103/101/104.
For example, original registry rows `0xF70`, `0x1F90` and `0x3880` selected
layout 321 / `0x29B00001`; rows `0x10C0`, `0x20E0` and `0x39D0` selected
331 / `0x29B00002`. The instance number was the suffix of
`isgrp_NNNNNN`, not the serialized node ID.

Two concrete joins illustrate that distinction. Offsets in this paragraph
are relative to the declared lyb payload base:

- In `0x29B00001`, payload base `0xD5B0`, UnitTree
  `sgrp_f0t0_a0_hlw1a_h` at `0x44CC0` had `show`/`hide` children targeting
  `time_bg_hlw1a_show` / `time_bg_hlw1a_hide` at `0xF167C` / `0xF16A4`.
  Instance `isgrp_003407` at `0x8BE70` referenced that tree. Its serialized
  node ID was 946; the native binding input was 3407.
- In `0x29B00002`, payload base `0x62B0`, Starlight UnitTree
  `sgrp_w_itm0_pstr1_h` at `0x24530` had `show`/`hide` children targeting
  `time_bg_crs1_show` / `time_bg_crs1_hide` at `0x5EAF0` / `0x5EB18`.
  Its instances were `isgrp_005414`, `005415` and `005416` at
  `0x486E0`, `0x48720` and `0x48760`.

The long-alias exception was also structural. Gridania interior owner
`sgrp_f0i0_w1_hlw03_h`, payload-relative `0x24120`, instance
`isgrp_005401` at `0x485A0`, named its children
`time_bg_hww3_show_003` and `time_bg_hww3_hide_004`. A four-byte `show`
request did not equal either key. The remaining nine Halloween owners in
that layout used the same longer-key pattern; their exact owner/timeline
joins are in the [interior finding](city-interior-halloween-owner-joins.md).

## Native binding and manual dispatch

| Native locator | Original-byte observation |
| --- | --- |
| `0x0058D2F2`, opcode `0x00D8` arm | Read instance number at packet `+0x10` and u16 layout selector at `+0x14`. The binding path required actor `+0xB2C` to be null. |
| `0x00805790` | Stored the instance at map object `+0x68` and formatted `isgrp_%06d` at `+0x48`. |
| `0x00806950`, ready branch `0x00806A52` | Required state at least 5, then looked up kind `0x12` using that instance name. Deferred paths `0x00806550` / `0x00806670` used the same name. |
| `0x0058D307`, opcode `0x00D9` arm; `0x00585320` | Required the retained binding and copied exactly four scheduler-name bytes from packet `+0x10`, followed by a terminator. |
| `0x00794690`, `0x0059D590` | Routed a name with an alphanumeric fourth byte, including `show`/`hide`, through scene selector `0x28`. |
| `0x008064A0`, `0x00806320`, `0x00802020` | Resolved the bound map object and matched its registered child alias byte-for-byte. |
| `0x008012F0`, `0x008024C0` | Required a kind-7 resource interface and its slot-11 result, then dispatched action `0x7F` through interface slot 3 at `0x0080256B`. |
| `0x0080256B`, admitted TimeLine manipulator | Action `0x7F` called the same slot-30 manipulator H's slot 3, conditionally bound to `0x00A77D10`. The separate clip callback `0x00836730` was not this action's direct target. |

The Lua exports did not bypass the short-name limit. Registration wrappers
`0x00737E30` / `0x00737F80` associated `_runBgScheduler` /
`_runBgSchedulerFromMidstream` with handlers `0x006F38D0` / `0x006F3BA0`.
Their literals were at `0x00FD6F1C` / `0x00FD6F2C`. Calls at
`0x006F3ADC` / `0x006F3D05` converged on `0x0075C8E0`, then
`0x00585320`, which staged one dword as the name. The dispatcher selected
a three- or four-byte name path. Thus neither export supplied the complete
long Gridania interior aliases above.

The later native message had more capacity. Builder `0x0059F460` copied
up to 31 name bytes into a 32-byte field and forced its final byte to zero.
The `0x34`-byte record had the packed binding handle at `+0`, action at
`+4`, zero at `+8`, name at `+0x0C`, and two optional copied dwords at
`+0x2C/+0x30`. Scene selector `0x28` reached arm `0x0062D041`, then
`0x008064A0` / `0x00806320`; `0x00805080` copied the full 13-dword record
into its retained request before exact child-key lookup.
Thus `time_bg_hww3_show_003` and `time_bg_hww3_hide_004`, both 21 bytes,
fit that native record. A qualified native producer could avoid the earlier
short-name restriction; this did not establish a stock packet/Lua bypass or
successful instance/scheduler admission. Those remain separate conditions.

This path did not itself read the weather vector or queue ID. It therefore
provided a candidate manual invocation when the weather mask was unselected,
subject to every bind, readiness, property and scheduler guard. The
[concrete manual admission and phase stores](timeline-weather-record-view.md#manual-action-and-phase-stores)
bind slot 11 to a context/child predicate and slot 3 to the wrapper's phase
and reset stores. A false predicate could skip the action while the request
consumer still returned AL=1. Successful registration and visible output
therefore remain separate from apparent request success.

### Persistence through updates

The reviewed manual action `0x7F` bodies had no direct stores to the weather
evaluator's G/F/L bytes or H+`0x20` flags. Downstream calls, including the
phase setter's S slot 11, remain separate possible side effects. The
[ordinary H-slot-24 update](timeline-weather-record-view.md#ordinary-update-and-pending-phase-branches)
ran before the evaluator gate and could re-enter the same phase/reset
method: a pending bit-1 branch required its predicate; separate baseline
and bit-2 branches had their own guards. A false bit-1 predicate retained
that pending flag and returned before the phase call. Stable weather alone
therefore proved neither persistence nor automatic overwrite.

The [weather-condition state machine](timeline-weather-condition.md#manual-controls-and-update-persistence)
separately retained the last selected bit and helper state. An unchanged
bit could still request a helper when that state required one; a settled
state did not unconditionally restart it. Clear/re-admission reset the
gate and helper state, retaining the last bit only in the same surviving
property object. No durable seasonal override was established for the manual
request. Actual opposing clip order and visible persistence remain
unwitnessed. Ul'dah interior's `hwa1` hide also used a material clip, so
the named pairs did not all imply the same visible operation.

The [retained-request reset contract](seasonal-decoration-update-order.md)
binds a separate replay mechanism: reset saved completed requests' phase,
cleared their completion bytes and requested scheduler cleanup. A
surviving request could be attempted again after binding. That contract
separates the local manual-dispatch and weather-condition phases from
unproved total scene order and actual object survival through a reload.

### Binding a weather layout itself

D8 was not proved limited to resident decoration layouts. Helper
`0x0059F2A0` called `0x0059DF90`, which searched the current RegionInfo
through `0x0079B380` and its children through `0x0079ACF0`, with an
all-region fallback at `0x0079B660`. That child search compared `+0x58`
without a type-zero exclusion. The original registry positively contained
child ID 8027. The returned LayoutInfo's leading name fed `0x0059F190`;
the later `0x00626710` manager-name search covered both ordinary and
weather/sky manager collections. Registration and later readiness remain
separate gates; table lookup alone did not bind a live weather instance.

However, the three stock city-8027 wrappers' direct UnitTree child aliases
provided no `show`/`hide`-style short control. Typed scans found 49, 54 and
58 child aliases in Gridania, Limsa and Ul'dah respectively, all longer
than four bytes. For example, Gridania's `sgrp_dwev_se` at file `0x4DB0`
exposed `time_xmas_se`, targeting `time_dwev_se` at `0x8B08`;
`sgrp_wtr_24` at `0x4D60` exposed `time_sqnc_24`, targeting `0x8CC0`.
Its separate `cbind_xmas` instance had a nonnumeric name. These bytes did
not provide an ordinary D9/Lua short-alias command to hide only the Xmas
effects or choose individual lighting curves. This scan was bounded to
the direct UnitTree child keys in these three wrappers.

## Weather inputs and direct-key boundary

Both packet `0x000D` and the
[Lua `_setWeather` bridge](../net/weather-transition-runtime.md#lua-weather-id-setter)
selected a 16-bit weather ID through scene selector `0x0C`. They did not
take an arbitrary resource key or a separately supplied light/effect list.
The Lua bridge truncated its first argument to a word; passing
`0x29B00020` supplied ID 32, not that weather resource.

Scene selector `0x0D` was a separate native operation. Its consumer
`0x00624240` stored a resource key, reverse-resolved an ID through
`0x0079B540`, and submitted the key to `0x007E87F0`. No authenticated server
packet or recovered Lua bridge producing that selector was established.
It remains a possible native interception target, not a proven server-only
control. It also did not manufacture resident decoration conditions.
The [direct-key state contract](../net/weather-transition-runtime.md#direct-resource-key-requests-and-retained-state)
binds the operation's native entry, retained fields, same-ID early exit,
ID-only state-3 deferral and explicit clear behavior.

The original registry made the seasonal key/ID distinction concrete:

| Stock resource | Root / original row | Reverse ID with Gridania root 103 preferred |
| --- | --- | ---: |
| Gridania Xmas-marked `0x29B00020` | 103 / `0xDF0` | 8027 |
| Limsa Halloween `0x29D9001F` | 101 / `0x2B0` | 8027, through all-root fallback |
| Ul'dah Halloween `0x615A0023` | 104 / `0x1450` | 8027, through all-root fallback |

Python `struct` followed the original registry's complete declared root and
child runs; the native preferred/all-root helpers were checked against the
pinned PE. Selecting different existing atmosphere keys therefore did not
separate the recovered decoration ID: all three still supplied 8027 to the
condition evaluator. Cross-city atmosphere resolution and rendering remain
unqualified, while the native key-selection input itself is established.

The original region registry had no 8033 row. Sending that ID alone
therefore did not select a new seasonal resource. The
[native lookup finding](native-seasonal-resource-selection.md)
owns normalization, resource-key lookup and ordinary DAT file loading.

## Composition candidates and remaining discriminator

The [native stock-DAT profile](native-stock-seasonal-switch.md) specifies
existing weather keys, a conditional spatial-name alias and an independent
resident-condition interception point. These are concrete extension choices,
with their receiver, reload and cross-city admission requirements preserved.

Selecting ordinary weather and manually showing retained Halloween owners
would test decoration control independently of weather. It would not by
itself restore the earlier Halloween lighting. Selecting Gridania's stock
Xmas-marked 8027 resource while manually hiding Halloween and showing winter
owners would test a separate Christmas composition. Both are static
candidates, with the dispatch guards and persistence boundary above.

A stock-only replacement requires an authenticated selector or native hook
for the intended atmosphere, successful instance/scheduler bindings, and
evidence that the intended decoration state survives ordinary updates,
weather transitions and region reloads. A resource-key hook reusing existing
DATs would avoid new asset files; it would still be a client-code change.
No such implementation or runtime test was performed here.
