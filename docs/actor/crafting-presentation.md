# Crafting presentation routing

The FFXIV 1.23b client owns class/tool-specific crafting motions, persistent
material and element effects, and transient result effects. These surfaces are
presentation consumers. They do not determine progress, durability, quality,
success, inventory changes, or recipe eligibility.

Native addresses refer to the pinned executable with image base `0x00400000`
and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Crafting corpus and motion banks

The decoded crafting inventory contains 875 files, 1,453 schedulers, 7,884
clips, and 864 motion headers, with no parse failures. Category 16 resolves its
character bank relative to the actor's equipped craft/tool directory:
`alc_alc`, `arm_arm`, `blk_blk`, `cok_cok`, `gld_gld`, `lth_lth`, `sew_sew`,
or `wod_wod`.

The packed selector retains the shared layout:

```text
(category << 24) | (characterBank << 12) | effectBank
```

The principal motion banks are:

| Presentation | Main-hand bank | Off-hand bank | MTB timing |
| --- | ---: | ---: | ---: |
| Rapid | 0001 | 0004 | 150 frames at 30 fps |
| Standard | 0002 | 0005 | 150 frames at 30 fps |
| Careful | 0003 | 0006 | 150 frames at 30 fps |
| Ability B | 0009 | 0010 | 80 frames at 30 fps |
| Completion joy | 0101 | 0101 | 135 frames at 30 fps |
| Completion upset | 0102 | 0102 | 240 frames at 30 fps |

The resource graph proves authored motion duration, not when the server accepts
another command or when the UI unlocks. Wait is supplied separately from the
main/off-hand motion selection. Ability B retains effect bank 2, while
completion joy/upset use their distinct completion banks.

## Persistent material and element state

Opcode `0x0144` supplies the packed crafting state in its first dword. The
client queues it through `0x007BF270` and applies it through `0x007BB740`.
Byte 1 is the server-authored `SubState.chantId` field.

Native functions `0x0065FFB0` and `0x0065AC70` decode three independent
groups:

| Packed bits | Effect IDs | Names |
| --- | --- | --- |
| `(value >> 14) & 3` | 902-905 | `cft_base`, `cft_base_md`, `cft_base_lg`, `cft_base_ch` |
| `(value >> 12) & 3` | 900 or 906-908 | clear, `gkrs_md`, `gkrs_bg`, `gkrs_gg` |
| `(value >> 8) & 15` | 900 or 909-924 | clear or `gkab_*` element effects |

The first six named element entries are `gkab_fire`, `gkab_ice`, `gkab_wind`,
`gkab_eart`, `gkab_thun`, and `gkab_wate`. The client updates actor fields
`+0x117C`, `+0x1180`, and `+0x1184` with associated dirty bits.

The consumer at `0x0065AD20` packs internal category 127 with the selected
effect ID and queues request types 22, 23, and 24 through `0x00846080`.
Resolver `0x00798470` sends categories at or above 111 through the loaded
scheduler's name-based entry point. This is why category-16 character motion
files alone do not contain the persistent material, spark, and elemental
effects.

## Resident effect banks

The shared resource banks are hash-identified:

| Resource | Bytes | Resources | Craft paths | SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `data/15/D9/00/00.DAT` | 4,973,264 | 1,177 | 159 | `10b8d7dc501a14a640e8338b0e17d2cc521756583fe4ed68859071d004b453e6` |
| `data/15/D9/00/01.DAT` | 1,189,004 | 279 | 31 | `9b1fca85933590e6e4d66a1b670408383519ef70b788a5118c3d61cc3c1df891` |

Both contain scheduler labels for all four material variants. Their authored
VEFF labels are:

| Variant | Scheduler | Authored layer label |
| ---: | --- | --- |
| 0 | `cft_base` | `cft_base1` |
| 1 | `cft_base_md` | `cft_orange1` |
| 2 | `cft_base_lg` | `cft_red1` |
| 3 | `cft_base_ch` | `cft_rainbow1` |

Red and rainbow are explicit asset names. The base color, the gameplay meaning
of the orange-authored layer, and which packed resource bank is resident in a
given session remain unresolved. Asset labels do not prove a white/yellow/red/
prismatic gameplay mapping.

The native owner route reaches `0x00798470`, then queue parent
`0x00844660`, which constructs a 16-byte resource key through `0x0062E2D0`
and looks up resource type `scb` in the actor's resident bank. This proves
name-based scheduler ownership but not the active DAT bank.

## Transient result effects

`0x007A1810` decodes crafting effect type 12, encoded with high value
`0x60000000`, from `CommandResult.effectId`. Its independent fields map as:

| Field | Value | Scheduler |
| --- | ---: | --- |
| Bits 8-13 | 1 | `gkra_succ` |
| Bits 8-13 | 2 | `gkra_grea` |
| Bits 8-13 | 3 | `gkra_faul` |
| Bits 5-7 equal 1, low bits | 1 | `gkac_nq` |
| Bits 5-7 equal 1, low bits | 2 | `gkac_hq1` |
| Bits 5-7 equal 1, low bits | 5 | `gkac_cf` |

These result selectors are separate from combat hit flags and from the
persistent `SubState.chantId` groups. Other variants are capabilities
only until a native result mapping selects them.

## Evidence boundary

The evidence establishes category-16 bank routing, class/tool-relative motion
families, exact MTB timings, persistent state decoding, resident scheduler
names, and transient result selectors. It does not establish retail command
cadence, action success formulas, quality or durability changes, material-color
semantics, server publish frequency, active resident-bank choice, or transaction
authority. Those behaviors must not be inferred from motion duration, effect
names, or resource availability.
