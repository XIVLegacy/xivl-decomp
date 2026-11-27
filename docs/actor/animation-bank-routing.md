# Character and effect animation bank routing

This note documents how packed animation selectors choose character and
effect banks in the pinned FFXIV 1.23b executable. The image has base
`0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Packed selector

For ordinary categories, the native selector consumed by `0x00798470` is:

```text
(category << 24) | (characterBank << 12) | effectBank
```

Category `0x0B` instead preserves the low 24 bits as one value and sets the
middle output to zero. The [ID catalog](emote-animation-ids.md#packed-playback-selectors)
records the exception-aware representation and native category-table boundary.

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
| 25 | `atk` | 17 | Category-name route; no automatic bow-shot binding established |

For route kind 7, `0x00798bf0` selects `em1` for posture values 0, 2, 15, and
91; `em2` for 11; and `em3` for 13 and 14. Unsupported posture values do not
load the normal item character bank. `0x00799c90` loads
`/client/vfx/itm/%04d.bin` for route kinds 6 and 7 only when the low
`effectBank` field is nonzero.

The [character-bank root publication path](../resource/actor-motion-resource-provider.md#character-bank-request-and-root-publication)
distinguishes the pending load handle from a decoded root and traces
conditional insertion into the owner's resource lookup vector. The request
record index is separate from that vector's insertion position; actual
delivery and retained bank/actor state remain required.

## Effect bank path

STATIC. For a category below `0x6F` other than `0x0B`, `0x00798470` splits
the selector at `0x00798510..0x0079851A` into the character bank
`(selector >> 12) & 0xFFF` and the effect bank `selector & 0xFFF`. For
category `0x0B` the effect bank is the low 24 bits and the character bank is 0
(`0x00798490..0x00798497`). Categories from `0x6F` up take other branches.
Both admitted forms call the row reader `0x007982D0` at `0x007984B3`. The
reader returns three fields of the 12-byte row at `0x00FE32D8 + 12 * category`:
the character flag byte at row `+4`, the effect flag byte at `+5` and the
route kind dword at `+8` (`0x007982F2..0x00798320`). A category above `0x22`
returns both flags clear and route kind `0x12`. When the character flag is
set the router calls visual slot `+0x54` with (category, character bank,
route kind), and when the effect flag is set it calls slot `+0x58` with
(category, effect bank, route kind) (`0x007984BB..0x007984F0`).

On the `CharaActionVisual` vtable `0x00FE4544`, slot `+0x58` (cell
`0x00FE459C`) is `0x00799C90` and slot `+0x5C` (cell `0x00FE45A0`) is
`0x00798640`. `0x00798640(category)` returns the row name pointer at
`0x00FE32D8 + 12 * category` for a category up to `0x22` and 0 otherwise.

`0x00799C90(category, bank, kind)` returns false for a category or bank that
is not positive (`0x00799CCB..0x00799CE5`). Route kinds 10 and 11 go to the
`gl%1d` builder `0x007997B0` instead (`0x00799CF2..0x00799CFE`,
`0x00799FB6`). Otherwise the path is built from these literals and calls:

| Part | Source |
| --- | --- |
| `/client/vfx/` | `0x00FE3C00`, at `0x00799D23` |
| folder | `itm` (`0x00FE3C10`) for route kinds 6 and 7, `abl` (`0x00FE3C14`) for kind 12, otherwise the slot `+0x5C` name, and false when that name is null (`0x00799D5B..0x00799E25`) |
| `/` | `0x00FE3C18`, at `0x00799E5B` |
| bank | `%04d` (`0x00FE3C1C`) of the bank argument, formatted at `0x00799EB4` |
| `.bin` | `0x00FE3C24`, at `0x00799F16` |

Row 1 holds the name `mgc` (`0x00FE3254`), both flags set and route kind 9.
Row 15 is the `pop` row recorded under
[Arrival type and revival motions](#arrival-type-and-revival-motions). So
selector `0x01000066` (category 1, effect bank `0x066`) forms
`/client/vfx/mgc/0102.bin`, and category 15 effect bank 7 forms
`/client/vfx/pop/0007.bin`. The installed files are `client/vfx/mgc/0102`
and `client/vfx/pop/0007`. The mapping from the `.bin` request name to the
installed path is outside this trace.

## Posture source and snapshot

STATIC. The kind-7 loader `0x00798BF0` reads the retained dword
`V+0x28` at `0x00798C2D` and `0x0079906C`, where V is its
`CharaActionVisual` receiver. It does not read actor posture directly
at those instructions. Vtable `0x00FE4544`, slot `+0x54`, contains
this loader at cell `0x00FE4598`. Slot `+0x48`, cell `0x00FE458C`,
contains setter `0x00799FD0`. The complete setter
`0x00799FD0..0x00799FDA` sign-extends its argument's low byte,
stores the dword at `V+0x28`, and returns with `RET 4`.
Construction initializes that dword to `0xFFFFFFFF` at `0x0079841B`.

Let C be the action controller and A its retained actor under the
[constructor owner profile](../resource/actor-motion-resource-provider.md#constructor-owner-profile).
The two checked queue-construction paths copy the same actor byte
into their visual through that setter.

| Path | Actor and byte sample | Constructor input and retained visual | Setter call |
| --- | --- | --- | --- |
| Ordinary submission `0x00845E80` | `C+0x228` at `0x00845F1F`, byte `A+0xB39` at `0x00845F2B` | Bundle `+0x1C` at `0x00845F78`, constructor `0x00843B50`, visual at queue `+0x14` | `0x00843CBB..0x00843CC8` |
| Preload `0x00845D70` | `C+0x228` at `0x00845D99`, byte `A+0xB39` at `0x00845DA5` | Bundle `+0x1C` at `0x00845DF8`, constructor `0x008438D0`, visual at queue `+8` | `0x00843A13..0x00843A20` |

Both constructors call `0x00798AA0`, whose store at `0x00798AE7`
installs V's primary vtable. They zero-extend bundle `+0x1C` before
calling virtual `+0x48`. For `S=A+0x590`, the source `A+0xB39`
is committed `S+0x5A9`. Selected `S+0x5A8` and desired `S+0x5AA`
are separate fields in the [posture producer](locomotion-selection.md#posture-inputs-and-selected-machine).

The ordinary sample follows conditional helper calls at
`0x00845EDB..0x00845F1F`. The checked path requires
`0x0065DF20` false, `0x0065DF30(1)` false or
`0x008440A0(record)` true, and `0x00844FF0(record)` false.
Preload samples before its `0x0065DF20` call at `0x00845DFC`
and requires that result false before construction. Both paths
require allocation and construction to succeed. The
[action queue](action-queue.md) owns the remaining admission and
consumption conditions.

The copied committed byte, after the setter's sign extension, is the
posture input retained for the checked kind-7 load. A later actor-state
commit alone does not cause a fresh actor read in that loader.
Unsupported retained values gain no standing fallback, and `-1` fails
the loader's unsigned `0x5B` bound. Other setter calls, direct writers
or replacement may change V before loading. These paths establish no
fixed sample-to-load latency, live queue delivery, bank availability,
actor selection or successful playback.

The cited instructions, vtable cells and route tables were checked in
Ghidra CLI 0.2.2 and independently decoded with LLVM 21.0.0 against
section-backed bytes of the pinned PE. This is static evidence.

## Route kinds 2 to 5

[Chara scheduler release](chara-scheduler-release.md#route-kinds-2-to-5) owns
the character bank directory, candidate loop and path parts for the `liu`,
`lin`, `lif` and `lil` categories.

## Attack category and command identity

The `CharaActionVisual` vtable at `0x00FE4544`, slot `+0x5C`, contains
`0x00798640` at cell `0x00FE45A0`. The route-17 branch of `0x00798BF0`
calls that slot at `0x00798E14`. The complete getter
`0x00798640..0x0079865A` reads the category-name table. Category 25's record
at `0x00FE3404` has name pointer `0x00FE32B4` (`atk`), character/effect
flags 1/1 and route kind 17. This is category-name selection, without a
weapon-specific automatic-shot discriminator at that call.

The cell, category record/name and complete instruction unions of the getter
and caller (`0x00798BF0..0x00799723`) were checked against the pinned original
PE with pefile 2024.8.26 and Capstone 5.0.7, using retained Ghidra assembly
locators. That check covers 8 getter instructions / 27 bytes and 606 caller
instructions / 2,868 bytes. It is static evidence, not observed playback.

Command labels are a separate namespace. The extracted catalog has Light Shot
at IDs 22108 and 27259, both mapped to `ShotCommand`, and Heavy Shot at
27228 and 27579, mapped to `AttackWeaponSkill` and `AttackCommand`
respectively. These static identities do not establish which command an
automatic bow attack executes (`xivl-client-data:derived/command_battle_params.csv`,
those row IDs, and `manifests/staticactor_class_paths.json`).

The 622-target-row retained battle-result dataset contains 158 rows with
command 22104 and packed selector `0x19001000`, including row_index 10 in
`combat_autoattack.pcapng`, frame 20, subevent 1. This splits to category25,
character bank1 and effect bank0. None of those target rows has any of the
four shot command IDs above. The dataset has no gear join for the witness,
and zero-target packets contribute no target rows. It therefore establishes
neither an automatic bow-shot mapping nor a supported replacement selector.

Source: `xivl-captures:studies/battle-result-backfit/derived/battle-result-rows.csv`,
SHA-256 `d67cb49d9489df127b3f374ee85792c43a6feffecbd4e2314072c0622b22e494`,
and the study's `derived/accounting.json`. The selected command table SHA-256
is `bc043bbd5558916a971de4d3a3a8dac5ec9d8ca36571bb534d0e964cd0b55d6a`.
The action/result fields and queue remain separate in the
[action-queue finding](action-queue.md) and
[combat-result finding](../net/combat-result-element.md). Deciding whether
one selector can select a bow motion requires the same actor's equipped
appearance, command/result context and selected resource/clip to be joined.

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

`0x00844330` searches the loaded resources for `main` and then a second name
that comes from visual `+0x224`. Its runtime value is unknown, and the
instructions do not show an empty name. See
[second scheduler name](chara-scheduler-release.md#second-scheduler-name).
A packed selector with low field zero can load an eat or drink
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
and 21 past the call at `0x0058a667`. The compare with 1 at `0x0058a63f` does
the same for type 1. [Arrival fade in](#arrival-fade-in) owns that call.
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

### Revival scheduler records

MEASURED, install read of `client/vfx/mgc/0102` with
`tools/scb_clip_scheduler.py` and the SEDBRES directory fields of the
[GTEX and PWIB loader](../resource/gtex-pwib-loader.md). The SEDBRES
directory has five entries: `main\bin\main` (an `SEDBSCB` of 1,856 bytes),
`abi2reis1tar\bin\abi2reis1tar` (an `SEDBSCB` of 1,296 bytes),
`sch_effect_data\reise02` (a nested `SEDBRES` of 67,145 bytes),
`RESOURCE_TYPE` and `RESOURCE_ID`. The clip records of `main`, in record
order with their start words:

| Clip class | Start word |
| --- | ---: |
| BindActorClip, RaptureServerMoveStopClip, RaptureClientMoveStopClip, RaptureActionSelectClip | 0 |
| RaptureCasterSchClip | 20,000 |
| RaptureCasterManagedSchClip | 120,000 |
| MotionClip | 170,000 |
| RaptureActionSubStatusSchKickClip | 1,100,000 |
| ClipSyncClip | 1,480,000 |

The MotionClip index `s32(D+0x10)` is 1, and entry 1 of the `RIDTBL`
resource of `main` is `cbnm_revive` by the
[RIDTBL lookup](../resource/combat-motion-name-provider.md). The
`abi2reis1tar` scheduler holds BindActorClip, two RaptureCharaColorFadeClip
(start words 0 and 170,000), RaptureSoundClip and ActionClip (180,000) and
RaptureActionSelectDamageMccClip (1,030,000). The start words are scheduler
ticks. Their conversion to frames is in
[Cut scheduler clip timing](../resource/cut-scheduler-clip-timing.md).
`client/chara/pc/c001/act/emp_emp/bid/base/0000` also holds the name
`cbnm_revive`. The c003 entry and its MTB extent are in
[Fall and revival entries](locomotion-bank-census.md#fall-and-revival-entries).

### Receive case, immediate arm and arrival stages

STATIC. Capstone 5.0.7 x86-32 linear disassembly of the pinned PE, with the
jump-table cells read from its section bytes. `0x0058b2a0` is not a function
in the Ghidra project, so its range was decoded from the raw bytes. Message
offsets below are payload offsets plus `0x10`, as in the section above.

The `0x00ce` case at `0x0058ce03` compares the element's actor ID
(`0x004D6750`, `0x0058ce05`) with the parent container's self actor ID
(`0x004D7490` on element `+0x7c`, `0x0058ce0f`), the getters of
[E2 selector state](../net/e2-selector-state.md). On equality it clears bit
0 of element byte `+0x26d` (`0x0058ce18`). If the u16 spawn type at message
`+0x34` is then `0x16` (`0x0058ce1f`), it calls `0x00586fb0` with element
`+0x7c` (`0x0058ce2a`) and leaves the case, with no position stored and no
call to `0x0058b2a0`. Every other message stores the position at message
`+0x18..+0x20` and 1.0 (`0x00f54f70`) at element `+0x214..+0x220`
(`0x0058ce58`, `0x0058ce7a`). It then calls `0x0058b2a0` with that
position, the rotation at message `+0x24`, the vector at message
`+0x28..+0x30`, the spawn type and the zoning flag at `+0x36`
(`0x0058ceed`).

`0x0058b2a0` first tests whether its element is the one `0x004D86B0`
returns (`0x0058b2aa..0x0058b2b5`), the
[nearest-target](../net/target-cycle-and-pick.md#nearest) player element.
For that element with flag 1, a spawn type 0 becomes 1
(`0x0058b2f1..0x0058b300`). The switch then compares `type - 1` with `0x17`
at `0x0058b2da`. Above it, so for type 0 and types past 24, it jumps to
`0x0058b366`. Otherwise byte table `0x0058b464` picks one of the two
entries of pointer table `0x0058b45c`, `0x0058b302` and `0x0058b366`. The
24 table bytes are 0 for types 1 to 10 and 15 to 24, and 1 for types 11 to
14.

The immediate arm `0x0058b366..0x0058b412` posts internal actor messages
`0x24` (the rotation as the second lane of a zeroed vector), `0x25` (the
position) and `0x26` (the message `+0x28` vector) through `0x004D7980`. It
stores that last vector at element `+0xb0..+0xbc`
(`0x0058b3dd`, `0x0058b3ea`). No arrival stage runs.

The arrival arm `0x0058b302` calls `0x0058adc0` with the type, the flag,
the position, the rotation and the `+0x28` vector. When stored flag
`+0xe6` is 1, `0x0058adc0` replaces only the destination: position at
`+0xc0..+0xcc`, vector at `+0xd0..+0xdc` and rotation at `+0xe0`
(`0x0058ae15..0x0058ae5d`). Otherwise it calls `0x0058a7f0`, clears the
stage bits of `+0xe8` and stores the type, the flag and the same
destination (`0x0058ae60..0x0058aecb`). It also writes fade byte `+0xeb`,
which is `0x3c` for type `0x17` and 10 otherwise. Back in `0x0058b302`, a
stored type of 1 calls `0x005878a0` at once (`0x0058b349..0x0058b355`)
before the state machine `0x0058a090` runs.

The state machine indexes table `0x0058a7a8` with
`([this+0xe8] >> 1) & 0x1f`. "Local" below is the branch taken when the
element equals the `0x004D86B0` element (`0x0058a0e2..0x0058a0f3`). A stage
that does not return early advances the stage bits.

| Stage | Entry | Behavior |
| ---: | --- | --- |
| 0 | `0x0058a19d` | posts `0x27 {0,type}` |
| 1 | `0x0058a1dd` | local: calls `0x00589000`, then `0x004D6FE0(1, 1)` at `0x0058a204` when bit `0x80` of `+0xe8` is set, else the fade `0x005606B0` with 0, 0, 0, 1.0 over byte `+0xeb` (`0x0058a22e`) |
| 2 | `0x0058a238` | local: waits while `0x004D70F0` returns true under bit `0x80`, else while `0x0055DB00` returns true |
| 3 | `0x0058a274` | local with stored flag other than 1: clears bit 0 of `+0xe8`, posts `0x27 {1,type}` and calls `0x005878a0` (`0x0058a2b6`). Not local: calls `0x005878a0` (`0x0058a2c2`) |
| 7 | `0x0058a3ad` | local with stored flag 1: posts `0x27 {1,type}` and calls `0x005878a0` (`0x0058a3ef`) |
| 12 | `0x0058a5cd` | local: the type-gated calls `0x004D70B0(1)` at `0x0058a60e` under bit `0x80`, else the fade with zeros (`0x0058a631`). Then the [30 frame fade in](#arrival-fade-in) (`0x0058a667`), and writes `0x14` to `+0xea` |
| 13 | `0x0058a69c` | local: counts `+0xea` down to 0, then calls `0x00589030` |
| 14 | `0x0058a6e6` | posts `0x27 {2,type}` |
| 15 | `0x0058a72e` | writes `0xffff` to `+0xe4` and 0 to `+0xe6`. Local: calls `0x00586fb0` with element `+0x7c` (`0x0058a778`) |

INFERRED. Every self `0x00ce` other than spawn type `0x16` moves the self
actor to the message position. Types 11 to 14, type 0 and types past 24 do
so at once through the posted messages. The other types do so in stage 3,
or in stage 7 with zoning flag 1, after the fade of stages 1 and 2. Type 1
also copies the destination before the first stage. Self type `0x16` calls
the routine that ends stage 15 and does not move the actor.

Reproduce with `ghidra disasm 0x0058cca0 -n 4000` filtered to
`0x0058ce03..0x0058cef2`, a Capstone decode of `[0x0058b2a0,0x0058b41a)`,
the 24 bytes at `0x0058b464` and the two dwords at `0x0058b45c`, decodes of
`[0x0058adc0,0x0058aecb]` and `[0x0058a090,0x0058a787]`, and the 16 dwords
at `0x0058a7a8`.

### Loading bit and the temporary gain

STATIC, the same decodes. `0x0058adc0` first writes `+0xeb = 10` and
`+0xe8 = (+0xe8 & 0xc0ff) | 0x80` (`0x0058adc3..0x0058ade2`). For type
`0x11` it then clears bit `0x80`, and for type `0x17` it writes `0x3c` to
`+0xeb` and clears bit `0x80` (`0x0058adeb..0x0058ae04`). Every other type
keeps bit `0x80`, the bit stages 1, 2 and 12 test.

The loading wrappers take the owner `+0x7c` in ECX and do nothing when the
owner's `+0x175a4` is null:

| Wrapper | Body |
| --- | --- |
| `0x004D6FE0(a, b)` | `0x0056A5F0(a, b)` on the `+0x175a4` object, which adds 1 to its `+0x21c` (`0x0056a60c`), then owner virtual `+0x38` at `+0xb4`, `0x0091C230(0)` and `0x004D6820(0.0, 0, 0, 0)` at `0x004D7025` |
| `0x004D70B0(a)` | `0x004D6820(-1.0, 0, 0, 1)` at `0x004D70CC`, then `0x0056A7C0(a)`, which writes 0 to `+0x21c` (`0x0056a7d8`) |
| `0x004D7110()` | byte `+0x34` of the `+0x175a4` object, false without one |
| `0x004D70F0()` | `0x00569F40` of the `+0x175a4` object, false without one |

The [temporary gain and restore](../resource/sound-effect-triggers.md#temporary-gain-and-restore)
section owns `0x004D6820`.

Stage 12 tests `0x004D7110` first (`0x0058a5e0`). Only when it is true and
the stored type is not `0x10` or `0x15` does it call `0x004D70B0(1)` under
bit `0x80` or the fade with zeros otherwise (`0x0058a5e9..0x0058a631`).

`0x0058a7f0` has two callers, `0x0058ae62` in `0x0058adc0` and `0x0058e8f7`
in the element destructor `0x0058e860`. It returns when `+0xe4` is already
-1. Otherwise it writes -1 to `+0xe4`, posts `0x27 {2,0}`, and for the
local element tests `0x004D7110` (`0x0058a845`). When that is true it calls
`0x004D70B0(1)` at `0x0058a866` under bit `0x80`, else the fade with zeros
(`0x0058a890`). The type compares with `0x10` (`0x0058a84e`) and 1
(`0x0058a895`) read the -1 just stored, so both pass. It then calls
`0x00589030` and clears bits 1 to 5 of `+0xe8` (`0x0058a8cb`).

`ghidra x-ref to 0x004d6fe0` lists callers `0x0058a204`, `0x0058a955`,
`0x0075b506`, `0x0088647c` and `0x006b115b`. `ghidra x-ref to 0x004d70b0`
lists `0x0058a60e`, `0x0058a866`, `0x0058aa57`, `0x0058ab0c`,
`0x0075b514`, `0x006b006b` and `0x006b11c2`. The callers outside stages 1
and 12 and `0x0058a7f0` were not traced.

INFERRED. For a local arrival with bit `0x80` the client shows Now Loading
and drops the temporary gain to 0.0 in stage 1, and restores it with the
screen in stage 12 or when `0x0058a7f0` ends the arrival early.

### Arrival fade in

STATIC. `ghidra` CLI disassembly of the same executable. The prefix
`0x0058a110..0x0058a19c`, which Ghidra does not list as instructions, was
decoded with Capstone 5.0.7 from the file-backed bytes. Both routines below
call `0x005606B0` a second time, with four floats of 0.0 and the constant 30
as the frame count. That routine is the fade setter of
`xivl-client-structs:docs/fade-actor-group-native-contract.md`. The same page
records four zeros as the floats of `_fadeIn`.

| Routine | Call block | Receiver | Skipped when the s16 at `+0xe4` is |
| --- | --- | --- | --- |
| Stage 12 of `0x0058a090` | `0x0058a636..0x0058a667` | EDI, which `0x0058a0ba` loads from the `0x004D7340` result for element `+0x7c` | 1 (`0x0058a63f`), `0x10` (`0x0058a645`) or `0x15` (`0x0058a64b`) |
| `0x0058a7f0` | `0x0058a89f..0x0058a8bf` | the `0x004D7340` result (`0x0058a8a2`) | 1 (`0x0058a895`) |

The other fade of each routine, at `0x0058a631` and `0x0058a890`, has the
same receiver. No instruction on the dispatch path from `0x0058a0ba` to
stage 12 writes EDI.

Both calls need the local element. In stage 12 the `0x004D7110` test
(`0x0058a5e7`), the type compares at `0x0058a5f2` and `0x0058a5f8` and the
bit `0x80` test (`0x0058a5fe`) guard only the earlier block. Its false arms
reach `0x0058a638`, ahead of the three compares above, directly or after the
fade at `0x0058a631`. In `0x0058a7f0` the `0x004D7110` test does not guard
the call either. The compare at `0x0058a895` reads the -1 stored earlier, so
it passes and the call runs for every early end of a local arrival. The call
of `0x00589030` follows at `0x0058a8c6`
([warp end release](../net/s2c-0193-native-state.md#warp-end-release)).

INFERRED. A local arrival that completes stage 12 with a type other than 1,
`0x10` or `0x15` requests a 30 frame fade in, with or without the loading
bit. The `_fadeInAfterWarp` routine `0x006E3260` calls `0x0075B500` and
`0x0075B300`, and none of the three calls `0x005606B0` directly. The arrival
call is therefore a static route that can clear an earlier script fade out.
Whether a script relies on it is not established. The consumer of
the fade packet is not traced, so the result of two calls in one pass, with
byte `+0xeb` and then 30 frames, is not established either.

Reproduce with `ghidra disasm 0x0058a090 -n 100`,
`ghidra disasm 0x0058a5cd -n 60`, `ghidra disasm 0x0058a7f0 -n 75` and
`ghidra x-ref to 0x005606b0`.

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
