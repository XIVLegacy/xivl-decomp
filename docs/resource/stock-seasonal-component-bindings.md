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
metadata, targets and curves require separate interpretation. Light-set
membership and pointed environment-map semantics were also unresolved.
This comparison establishes partial content retention, without claiming
either complete Halloween equivalence or its impossibility.

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
DrawEnv, instance, sound and texture bindings differed; neither matching
lane names nor different node counts establish cross-city compatibility or
incompatibility. Native selection and a correctly resolved/rendered result
remain unproven.

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
| `0x00836730` | Obtained the numeric instance through UnitInfo slot 50, the timeline scheduler through property slot 30, and conditionally invoked scheduler slot 3 at `0x0083683B`. |

The Lua exports did not bypass the short-name limit. Registration wrappers
`0x00737E30` / `0x00737F80` associated `_runBgScheduler` /
`_runBgSchedulerFromMidstream` with handlers `0x006F38D0` / `0x006F3BA0`.
Their literals were at `0x00FD6F1C` / `0x00FD6F2C`. Calls at
`0x006F3ADC` / `0x006F3D05` converged on `0x0075C8E0`, then
`0x00585320`, which staged one dword as the name. The dispatcher selected
a three- or four-byte name path. Thus neither export supplied the complete
long Gridania interior aliases above.

This path did not itself read the weather vector or queue ID. It therefore
provided a candidate manual invocation of a short-alias control when the
weather mask was unselected, subject to every bind, readiness, property and
scheduler guard. It did not prove successful registration or visible output.
In particular, slot 11's successful result must not be assumed.

The callback did not write a durable weather-override field. The separate
[weather-condition evaluator](timeline-weather-condition.md) could later
apply the authored show/hide condition again. The relative execution order
and persistence of a manual request were not witnessed. Ul'dah interior's
`hwa1` hide also used a material clip, so the named pairs did not all imply
the same visible operation.

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

The original region registry had no 8033 row. Sending that ID alone
therefore did not select a new seasonal resource. The
[native lookup finding](native-seasonal-resource-selection.md)
owns normalization, resource-key lookup and ordinary DAT file loading.

## Composition candidates and remaining discriminator

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
