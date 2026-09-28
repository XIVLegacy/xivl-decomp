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

## Raid-dungeon warp bank

The retail `RaidDungeonWarp.activateWarpDevice` client method requests
decimal scheduler 67493888 (`0x0405E000`), as recorded in
`xivl-client-scripts:docs/raid-object-client-contracts.md` under Object
behavior. The packed category is 4 (`lib`), the middle bank field is
94, and the low field is zero. The
`client/chara/bgobj/b936/act/cmn/lib/base/0094` resource is 1,232 bytes,
SHA-256
`ba71a7d66808704200a99eb508d306e8fae2ffe5fdd98bc4135f8f8f0dd74cd0`.
It contains `b936e004`, `initf_idle`, `BindActorClip`, and
`RaptureCancelChantSyncClip` literals. This joins the method's packed
selector to an authored e004 control bank when the receiving actor has
the compatible b936 resource family.

The separate e004 bank `0004` is 55,040 bytes, SHA-256
`4a12fc6f6076b919cf008c9f14a0215ed4b7774aee49b8fc43cc1c62b7dd48fa`.
It contains `b936e04v2` and `EffectClip`, but no recovered retail
caller in this evidence selects `0x04004000`. Bank `0014` is another
1,232-byte e004 control (SHA-256
`3af9353f36d0585d09e115a6965764321a0178962c4b17fe07731caa4984c414`);
it differs from `0094` at only four byte positions. Neither asset
presence nor the Lua call identifies a retail world actor ID, transport
destination, or phase-specific activation policy.

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
Resources contain distinct meat, cake, salad, egg, soup, apple,
cheese, and cookie effects. No recovered table in this evidence maps every
catalog item ID to its retail effect bank. `0x05fa1001` combines the generic
self-item character bank with the effect whose authored path contains
`item/drug/potion1/itm1_pot_f.veff`, but that does not prove it is correct for
every medicine.

## Arrival type and revival motions

Wire opcode `0x00ce` reaches `0x0058ce03`. The payload's spawn type at
`+0x24` and zoning flag at `+0x26` reach `0x0058b2a0`. Its non-flag-1 path
stores them at actor `+0xe4` and `+0xe6`. `0x0058a090` later emits internal
actor message `0x27`
with the spawn type during arrival stage 14. This `0x27` is an internal
message ID, not a network opcode.

Renderer code at `0x006631ce` selects a POP effect only for spawn types 2
through 10, plus type 23 remapped to 2. `0x0065aab0` and `0x0065aad0` pack
those as category 15 with character bank 0 and the spawn type as effect bank.
Spawn type 1 instead follows `0x00663201 -> 0x00661ab0(20)`, which configures
a color fade. The POP resources contain no `0001` bank. Thus spawn
type 1 does not itself select a resurrection or get-up motion.

For an accepted type-7 arrival, the same executable has a more
specific conditional path. `0x0058ce85/0x0058ce89` read the two arrival
words and `0x0058ceed` calls `0x0058b2a0`; its type dispatch reaches
`0x0058adc0`. That function's non-flag-1 branch stores type and flag at
actor `+0xe4/+0xe6` (`0x0058ae7b/0x0058aec3`); the flag-1 branch follows
a different return path.
The `0x0058a090` stage dispatcher emits internal message `0x27 {0,type}`
at `0x0058a19d`; renderer subtype 0 (`0x00663183`) calls
`0x00661990(0)`, whose mask-8 zero-target color update sets the fourth
color component to zero. Stage 3 (`0x0058a274`) has conditional calls to
`0x005878a0` at `0x0058a2b6/0x0058a2c2`; that function copies the stored
destination into actor position fields. Stage 14 (`0x0058a6e6`) emits
`0x27 {2,type}`. Renderer subtype 2 (`0x006631ce`) retains type 7 in its
2-10 range and passes effect bank 7 to `0x0065aab0`, under the existing
category-15 POP route. These are executable paths, not evidence that a
particular historical actor or event selected type 7.

`client/vfx/pop/0007` has SHA-256
`ffd87484a18d27242fc25e5202d7459702815063ec9e33873dc555e82474d950`.
Its three 60-byte color records at file offsets `0x358`, `0x3ac`, and
`0x3e8` have SHA-256 `17e57a3c1083f167d27bc74310608d78a44933d4b76bdc8724258b8404eddae2`,
`156dcfdb37b0cebed37e1bb8a204b210dba62f61cc2191993ea911ebfb83f9bf`,
and `35c39fd6e413c5fa246fd8a1c20a15a8f1e286c2f5bf23a531e10094c43b2423`.
They are `RaptureCharaColorFadeClip` records: zero alpha at start 0, a later
1.8/1.8/1.7 color target with alpha 1, and a later white target with alpha 1,
respectively. Raw timeline units do not establish wall-clock visibility or
combat targeting. The historical Chain Bearer arrival type and selector
remain unknown.

Row 15 of the 12-byte category table at VA `0x00FE32D8` is at VA
`0x00FE338C` (file offset `0x00BE338C`) with bytes
`8C 32 FE 00 00 01 00 00 0D 00 00 00`;
the first dword points to VA `0x00FE328C` (file offset `0x00BE328C`),
whose NUL-terminated name is `pop`. The row records character bank 0, VFX
bank 1, and route kind 13. This resolves the static category mapping, not
any encounter's selector.

The same asset's `outer/RES:main` resource is a 1,360-byte `SEDBSCB`
payload with SHA-256
`718af4c8b24cb3a10932f6a4ee31dfd44b1057e4d534e8ab9997706696e8ad19`.
Its `Block000` declares 990,000 raw timeline units. Its seven records are
`BindActorClip` at payload offset `0x260` (20 bytes), `RaptureSoundClip` at
`0x274` (44 bytes), `ActionClip` at `0x2A0` (40 bytes, referencing `pop7`),
`RaptureCharaColorFadeClip` at `0x2C8` (60 bytes), `ClipSyncClip` at
`0x304` (24 bytes), and `RaptureCharaColorFadeClip` at `0x31C` and `0x358`
(60 bytes each). The three fade
records' u32 parameter fields at payload offsets `0x2DC`, `0x330`, and
`0x36C` are 0, 10, and 20; these are authored values, not wall-clock timing.
These authored records do not establish that a historical actor selected
type 7 or that these clips rendered live.

The arrival dispatcher at VA `0x0058b2a0` accepts spawn types 1 through 10
and 15 through 24 in its switch. Type 15 therefore reaches the common setup
call at `0x0058adc0` and the state machine at `0x0058a090`; this establishes
native handling capability, not that any historical transport selected type
15 or used a separate scene. The dispatcher indexes its byte table at VA
`0x0058b464` with `type - 1`; entries 15 and 20 (types 16 and 21) both select
pointer-table entry 0 at `0x0058b45c`, which targets `0x0058b302`. That path
passes the selected type to `0x0058adc0` and then calls the state machine at
`0x0058a090`. The state index is `([this+0xe8] >> 1) & 0x1f`; the pointer
table at `0x0058a7a8` maps state 12 to `0x0058a5cd`.

In that state-12 path, comparisons at `0x0058a5f2` and `0x0058a5f8` route
types 16 and 21 past the conditional calls at `0x0058a60e` and `0x0058a631`.
The later comparisons at `0x0058a645` and `0x0058a64b` also route types 16
and 21 past the call at `0x0058a667` (which otherwise receives constant 30).
The shared tail writes `0x14` to actor byte `+0xea` at `0x0058a684` and
updates the state word. The instruction paths were checked in the Ghidra 12.1
disassembly; the switch-table entries were read from the raw pinned PE.
These branches establish native capability, not the semantics of each helper
or a historical selection. `RaidFst0Dungeon03`'s `rad0f300` call is a separate
client-script observation in
`xivl-client-scripts:docs/content-director-ui-contracts.md`; no cited evidence
joins that scene to spawn type 16 or 21.

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

The c001 FID bank `client/chara/pc/c001/act/cmn/fid/base/2072`
(SHA-256 `1e9b9fea8f61bea7127265c5dd6e6ddaa57db06fc5bd1432da204cb782ede05a`)
contains the persistent `fxpf_idle` scheduler and authored
`cbmm_h2_sud_b` motion references. The separate LIB bank
`client/chara/pc/c001/act/cmn/lib/base/1037`
(SHA-256 `a1f4d5a21329225c098299eb764e4d342488b3445d7f1b56c4581b8c736e028a`)
has a direct-action `main` scheduler referring to the same named motion.
FID bank 2013
(SHA-256 `ecffdb6b6abd35209f7b147c7c6ab63822a88facf81d0d6a4c70458eb71ac729`)
instead names `cbmm_h2_sud_a`, so it is not an asset-identical replacement.

Those file hashes were independently checked against the installed action
banks. The named scheduler and motion references are static asset records.
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

## Motion-command controller clock

`MotionCommandClip` update at VA `0x00DE7B40` (RVA `0x009E7B40`) loads a
signed timeline integer at `0x00DE7B5B`, divides it by the double
`10000.0` at VA `0x00FE0570`, and passes the float result to the motion
player through the virtual call at `0x00DE7B79`. These instructions and
the constant were checked directly in the pinned `ffxivgame.exe` with
`pefile` and Capstone x86-32. This controller uses 10,000 raw units per
motion frame, not microseconds. It does not establish a universal time
unit for other scheduler fields or clip classes.

The m020 idle, walk, and run MCB values can be compared with their MTB frame
counts, but authored replacement m521 motions and reported playback symptoms
do not establish a retail motion binding.
