# Court in the Sands cutscene actor records

The installed `client/cut/<scene>/<scene>` files for `man0u175` and
`man0u180` have the following byte identities. Each has one embedded
`SEDBSCB` header; its little-endian length at header `+0x10` bounds the
SCB digest. The surrounding PWIB is larger than that embedded scheduler.

| Scene | PWIB bytes / SHA-256 | SCB offset / bytes / SHA-256 |
| --- | --- | --- |
| `man0u175` | 30,752 / `12e83457a1f15491f19add5acefed9fb37f98d6a53e6efd775252d619153079e` | `0x80` / 30,512 / `371e217eb1e29482770b17d9fc8b0a382658f280bd18f2fb6243f71e7c9f51de` |
| `man0u180` | 724,464 / `8b93667f22535af91f2cab1b2a18d5bc4c8f8b7b16e141b33452be5861a8e82e` | `0x80C10` / 73,808 / `45e72fa4b5e6f9efb94b8ca88e40404217c91a428c7cc9b2f1b2efa7d9dc068a` |

A read-only scan decoded numeric actor records using the `0x3C`-byte
signature and field offsets described in
[raid cutscene numeric actor records](raid-cutscene-actor-records.md):
little-endian words at record `+0x20`, `+0x28`, and `+0x2C` equal `5`,
`0xFFFFFFFF`, and `2`; the token is at `+0x30`. The table lists the
nonzero numeric tokens and their authored labels. It excludes common
system/camera/sound/light records and unlabeled background proxies.

| Scene | Record offset | Index | Authored label | Numeric token |
| --- | ---: | ---: | --- | ---: |
| `man0u175` | `0x25C` | 5 | `FLhaminn` | 1000842 |
| `man0u175` | `0x298` | 6 | `EvisAdv` | 1000790 |
| `man0u175` | `0x2D4` | 7 | `WeakyAdv` | 1000793 |
| `man0u175` | `0x310` | 8 | `TerrorAdv` | 1000794 |
| `man0u175` | `0x34C` | 9 | `BeastyAdv` | 1000938 |
| `man0u175` | `0x388` | 10 | `KiraKiraAdv` | 1000798 |
| `man0u175` | `0x3C4` | 11 | `QuickAdv` | 1000799 |
| `man0u175` | `0x400` | 12 | `ManlyAdv` | 1000936 |
| `man0u175` | `0x43C` | 13 | `GleedyMerchant` | 1000937 |
| `man0u175` | `0x478` | 14 | `SunnyMerchant` | 1000939 |
| `man0u175` | `0x4B4` | 15 | `BZangho` | 1000805 |
| `man0u180` | `0x80E0C` | 5 | `dami_hf` | 1000812 |
| `man0u180` | `0x80E48` | 6 | `Villagers_A` | 1000808 |
| `man0u180` | `0x80E84` | 7 | `Villagers_B` | 1000809 |
| `man0u180` | `0x80EC0` | 8 | `CORGUEVAIS` | 1001054 |
| `man0u180` | `0x80EFC` | 9 | `FLHAMINN_kako` | 1000038 |
| `man0u180` | `0x80F38` | 10 | `ACilLIA` | 1000042 |
| `man0u180` | `0x80F74` | 11 | `dami_rrm1` | 1000813 |
| `man0u180` | `0x80FB0` | 12 | `dami_rrf` | 1000810 |
| `man0u180` | `0x81010` | 14 | `dami_ldob` | 1000814 |
| `man0u180` | `0x8104C` | 15 | `dami_hm` | 1000815 |
| `man0u180` | `0x81088` | 16 | `dami_rd` | 1000811 |
| `man0u180` | `0x810C4` | 17 | `dami_rrm2` | 1000816 |
| `man0u180` | `0x81100` | 18 | `thank` | 1000603 |
| `man0u180` | `0x8113C` | 19 | `_AETHERYTE` | 1200013 |
| `man0u180` | `0x81178` | 20 | `Ludovraint` | 1500073 |
| `man0u180` | `0x811B4` | 21 | `dami_` | 1500099 |
| `man0u180` | `0x811F0` | 22 | `Benedict` | 1600062 |
| `man0u180` | `0x8122C` | 23 | `Blandhem` | 1000672 |
| `man0u180` | `0x81268` | 24 | `Chechedoba` | 1000673 |
| `man0u180` | `0x812A4` | 25 | `FLHAMINN` | 1000842 |

`man0u180` therefore contains distinct `FLHAMINN_kako` and
`FLHAMINN` records, not a duplicate label inferred from a name match.

The `man0u180` SCB also contains 20-byte type-3 visibility records. Their
word at `+0x10` toggles between zero and one. Joining their actor index
and track to the dictionary above gives this direct authored sequence:

| Block | Record offset | Clip ID | Actor / track | Value at `+0x10` |
| --- | ---: | ---: | --- | ---: |
| `setup` | `0x81E1C` | 72 | `_AETHERYTE` / 53 | 1 |
| `setup` | `0x82EE8` | 176 | `FLHAMINN` / 20 | 1 |
| `setup` | `0x82FA4` | 182 | `FLHAMINN_kako` / 18 | 0 |
| `cut5` | `0x83F80` | 269 | `_AETHERYTE` / 53 | 0 |
| `cut5` | `0x84084` | 276 | `FLHAMINN` / 20 | 0 |
| `cut5` | `0x840D8` | 278 | `FLHAMINN_kako` / 18 | 1 |
| `cut6` | `0x84870` | 319 | `_AETHERYTE` / 53 | 1 |

The `0x40`-byte type-1 position records for `_AETHERYTE` track 53
place it at `(55.900002, 200.001587, -493.200012)` in setup clip 57
(`0x81BB8`), then `(34.900002, 200.001587, -480.200012)` in setup
clip 158 (`0x82BF0`). These are scene-authored positions, not persistent
world placement coordinates. The complementary type-3 values support a
F'lhaminn visibility swap and aetheryte hide/show staging change. They
do not by themselves prove a historical private-area transfer or the
retail server trigger for the scene. Dialogue timing, motion semantics,
and the narrative interpretation of the swap remain outside this audit.

## Action and message records

The `man0u180` resource-name table begins at `RIDT` offset `0x865D0`;
its 16-byte entries begin at `0x865F0`. Slot 40 is `kako_in1`
(`0x86870`), and slot 46 is `heal` (`0x868D0`). Two 40-byte type-29
action records refer to those slots:

| Block | Record offset | Clip ID | Local time | Actor / track | Resource slot | Flags |
| --- | ---: | ---: | ---: | --- | --- | ---: |
| `cut5` | `0x84034` | 274 | 0.00 s | `FLHAMINN_kako` / 18 | 40 / `kako_in1` | `0x00A0` |
| `cut6` | `0x84B50` | 337 | 1.95 s | `CORGUEVAIS` / 25 | 46 / `heal` | `0x02A0` |

At `0x84D2C`, a type-30 record in `cut8` has clip ID 345 at local time
zero and targets clip ID 337 in its word at `+0x14`. This is a direct
scheduled end reference to the `heal` action. The numeric flags and
action names do not establish a gameplay heal, health change, or exact
effect appearance.

Both SCBs contain 36-byte message-row records. The row is the word at
record `+0x14`; local time is the microsecond word at `+4`. The three
`man0u175` records are type 18 and all have flags `0x02A0`:

| Scene | Record offset | Clip ID | Local time | Text row |
| --- | ---: | ---: | ---: | ---: |
| `man0u175` | `0x1ADC` | 130 | 0.59 s | 144 |
| `man0u175` | `0x1C9C` | 141 | 0.42 s | 145 |
| `man0u175` | `0x1F24` | 155 | 0.06 s | 146 |

The thirteen `man0u180` records are type 24:

| Block | Record offset | Clip ID | Local time | Text row | Flags |
| --- | ---: | ---: | ---: | ---: | ---: |
| `cut2` | `0x8341C` | 210 | 0.75 s | 147 | `0x02A0` |
| `cut3` | `0x83A30` | 244 | 1.20 s | 148 | `0x02A0` |
| `cut5` | `0x84204` | 286 | 0.50 s | 149 | `0x02A0` |
| `cut5` | `0x843E0` | 297 | 1.06 s | 150 | `0x02A0` |
| `cut6` | `0x848B4` | 322 | 0.10 s | 151 | `0x00A0` |
| `cut8` | `0x84E74` | 354 | 0.10 s | 152 | `0x00A0` |
| `cut9` | `0x85110` | 369 | 0.12 s | 153 | `0x02A0` |
| `cut9` | `0x85160` | 372 | 0.60 s | 154 | `0x02A0` |
| `cut10` | `0x852A4` | 379 | 0.10 s | 155 | `0x02A0` |
| `cut11` | `0x85420` | 388 | 0.12 s | 156 | `0x00A0` |
| `cut11` | `0x85494` | 391 | 0.82 s | 157 | `0x00A0` |
| `cut12` | `0x85678` | 402 | 0.20 s | 158 | `0x02A0` |
| `cut13` | `0x85B7C` | 432 | 0.32 s | 159 | `0x00A0` |

These rows prove authored scene references, not spoken text, actor/speaker
ownership, live playback duration, or a retail quest transition. The
report's broader motion and camera interpretation remains to be checked
against the installed scheduler before promotion.
