# Character and effect animation bank routing

This note documents how packed animation selectors choose character and
effect banks in the pinned FFXIV 1.23b executable. The image has base
`0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Packed selector

The native selector consumed by `0x00798470` is:

```text
(category << 24) | (characterBank << 12) | effectBank
```

The function loads the character bank through vtable slot `+0x54` and the
effect bank independently through slot `+0x58`. The category table at
`0x00fe32d8`, read by `0x007982d0` and `0x00798640`, defines these relevant
routes:

| Category | Bank name | Route kind | Behavior |
| ---: | --- | ---: | --- |
| 5 | `emt` | 7 | Select `em1`, `em2`, or `em3` from posture; VFX uses `itm` |
| 6 | `em1` | 6 | Explicit standing character bank; VFX uses `itm` |
| 7 | `em2` | 6 | Explicit seated character bank; VFX uses `itm` |
| 8 | `em3` | 6 | Explicit ground-seated character bank; VFX uses `itm` |
| 1 | `mgc` | 9 | Magic character bank and magic VFX |
| 4 | `lib` | 1 | Library character bank and library VFX |

For route kind 7, `0x00798bf0` selects `em1` for posture values 0, 2, 15, and
91; `em2` for 11; and `em3` for 13 and 14. Unsupported posture values do not
load the normal item character bank. `0x00799c90` loads
`/client/vfx/itm/%04d.bin` for route kinds 6 and 7 only when the low
`effectBank` field is nonzero.

## Food and drink bank composition

The c001 standing character banks establish the motion side of the route:

| Resource | Scheduler and motion | SHA-256 |
| --- | --- | --- |
| `pc/c001/act/cmn/em1/base/4001` | `itm00`, `cbnm_itm_slf` | `d17b97c757e857f92a31014c4098e6dd14d33ad91864a935efa51a3c0933603a` |
| `pc/c001/act/cmn/em1/base/4002` | `itm00`, `cbnm_drink` | `53677f20d6c74f810dbbfd3dd274ae446ad30232e0bb1e06a93e6c84df5afda4` |
| `pc/c001/act/cmn/em1/base/4003` | `itm00`, `cbnm_eat` | `af9ca5ce652f646aa32eadc8f454b083abbb7f64921457ea2d88d69f2f455181` |

The eat and drink character banks do not contain a `main` scheduler. Their
`itm00` scheduler also calls `food_vfx`, which comes from the separately
loaded item-effect bank. The verified item parents are:

```text
client/vfx/itm/0101 or 0102: main
  -> item_main
  -> itm00 from the character bank
     -> cbnm_eat or cbnm_drink
     -> food_vfx from the item-effect bank
```

`client/vfx/itm/0101` is 141,360 bytes with SHA-256
`acd438bfb9c6350acad2f54640afb1f6d01309f1520a8245f56a1e65ae598760`.
`client/vfx/itm/0102` is 133,392 bytes with SHA-256
`680b284cab28f6f48c65c654fdef998b64e8a7268819dd4f10bb9b9b9a95516d`.
The latter preserves the authored drink prop path
`item/mesi/drink1/veff/bottle0.veff`.

`0x00844330` searches the loaded resources for `main` and then an empty-name
fallback. A packed selector with low field zero can load an eat or drink
character motion bank while omitting the authored parent and its item VFX.
The complete selectors backed by these two recovered parents are therefore:

| Presentation | Packed selector |
| --- | ---: |
| Bread parent plus eat motion | `0x05fa3065` |
| Drink parent plus drink motion | `0x05fa2066` |

Opcode `0x00da` dispatch at `0x0058cffa` reads the first payload dword and
calls `0x0058cad0`, then `0x0058c690`. This route constructs its own internal
single-target action context; its second on-wire dword is not read here.

The packed IDs prove the bank combination, not a universal item mapping.
Installed resources contain distinct meat, cake, salad, egg, soup, apple,
cheese, and cookie effects. No recovered table in this evidence maps every
catalog item ID to its retail effect bank. `0x05fa1001` combines the generic
self-item character bank with the effect whose authored path contains
`item/drug/potion1/itm1_pot_f.veff`, but that does not prove it is correct for
every medicine.

## Arrival type and revival motions

Wire opcode `0x00ce` reaches `0x0058ce03`. The payload's spawn type at
`+0x24` and zoning flag at `+0x26` reach `0x0058b2a0` and are stored at actor
`+0xe4` and `+0xe6`. `0x0058a090` later emits internal actor message `0x27`
with the spawn type during arrival stage 14. This `0x27` is an internal
message ID, not a network opcode.

Renderer code at `0x006631ce` selects a POP effect only for spawn types 2
through 10, plus type 23 remapped to 2. `0x0065aab0` and `0x0065aad0` pack
those as category 15 with character bank 0 and the spawn type as effect bank.
Spawn type 1 instead follows `0x00663201 -> 0x00661ab0(20)`, which configures
a color fade. The installed POP resources contain no `0001` bank. Thus spawn
type 1 does not itself select a resurrection or get-up motion.

The decoded resources expose these separate candidates:

| Selector | Resource evidence | Status |
| ---: | --- | --- |
| `0x01000066` | `client/vfx/mgc/0102` has `main`, `cbnm_revive`, stop clips, `initf_idle`, and a substatus kick | Verified revival scheduler bank |
| `0x04122000` | `pc/c001/act/cmn/lib/base/0290` has `main -> cbfm_getting_up`, 235 frames at 30 fps | Named slow get-up candidate |
| `0x04123000` | `pc/c001/act/cmn/lib/base/0291` has `main -> cbfm_body_up`, 190 frames at 30 fps | Named body-up candidate |

The magic bank `client/vfx/mgc/0102` is 152,612 bytes with SHA-256
`8222a9aa161e2b69c6be78741108267af321baf96f43b13241446de3881964a5`.
The c001 library banks 0290 and 0291 have SHA-256
`7482cc0af69d45f43e43485ff9882a2082fb9772c58a6070e2da9eb24a878bc2`
and `bf1aa7bcbe63840a1c98c0d2497e962a39e9d0a80c9ff3898ebd8f3553991ac8`.

The previously proposed `0x01001094` loads `cmn/mgc/base/0001` and effect
bank `mgc/0148`; the latter contains no `MotionClip`. It is not evidence for
the recovered `cbnm_revive` bank.

## Ferry steersman motion assets

The installed c001 FID bank `client/chara/pc/c001/act/cmn/fid/base/2072`
(SHA-256 `1e9b9fea8f61bea7127265c5dd6e6ddaa57db06fc5bd1432da204cb782ede05a`)
contains the persistent `fxpf_idle` scheduler and authored
`cbmm_h2_sud_b` motion references. The separate LIB bank
`client/chara/pc/c001/act/cmn/lib/base/1037`
(SHA-256 `a1f4d5a21329225c098299eb764e4d342488b3445d7f1b56c4581b8c736e028a`)
has a direct-action `main` scheduler referring to the same named motion.
FID bank 2013
(SHA-256 `ecffdb6b6abd35209f7b147c7c6ab63822a88facf81d0d6a4c70458eb71ac729`)
instead names `cbmm_h2_sud_a`, so it is not an asset-identical replacement.

Those installed-file hashes were independently checked. The scheduler and
motion observations come from the contributor's
`ferry_steersman_pose_investigation_2026-09-11.md`, Installed asset evidence.
The reported intermittent reference pose remains undiagnosed: asset
presence and named motion references do not prove successful playback,
identify a lifecycle failure, or authorize a periodic restart.

## Evidence boundary

Native routing and decoded scheduler edges prove the selector layout and the
contents of the candidate banks. They do not recover the original per-item
effect lookup, or establish which slow body motion retail Return used. Asset
names alone cannot select between those Return candidates. Scheduler raw units
also must not be called seconds merely because dividing by one million yields
a plausible number; the MTB frame counts are the direct timing evidence.
