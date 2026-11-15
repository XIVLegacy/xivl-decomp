# Character action sound clip

The attack and fall schedulers of the installed character banks play their
grunts and hit sounds through `RaptureCharaActionSoundClip`, not through
`RaptureSoundClip`. This page records slot 1 of that clip class, the actor
entry it calls, the CharaSoundController table the entry reads, the weapon
sound table that answers hit groups, and the paths that fill both tables.
The [sound effect triggers](sound-effect-triggers.md#entry-points-and-callers)
page owns the module entry points, and the
[footstep member](footstep-detector-and-ground-material.md#footstep-member)
section places the CharaSoundController at CharaActor `+0x21D4`. The layout
of the installed sound tables is
`xivl-client-data:docs/chara-sound-tables.md`.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
base `0x00400000`. All native addresses are VA.

Producing tools are `ghidra` CLI driving Ghidra `12.1.4` disassembly and
cross-reference queries, and reads of the file-backed PE sections for
strings, tables and vtables. Install measurements walk the RES directories of
`xivl-tools:docs/formats/sedb-res.md`, nested entries included, and parse
each `SEDBSCB` child with `tools/scb_clip_scheduler.py`. D is a clip record
as in the [clip layout](cut-scheduler-clip-layout.md#clip-records).
Everything native here is static.

## Class and record

STATIC. The RTTI catalog
([ffxivgame.rtti.json](../../config/ffxivgame.rtti.json)) gives
`Application::Scene::Cut::Clip::RaptureCharaActionSoundClip` the 42-slot
vtable at RVA `0xc2b63c` (VA `0x0102B63C`). Its slot 1 is `0x00829040`.

MEASURED. The 11833 files below `client/chara/*/*/act` hold 3874
RaptureCharaActionSoundClip records, every one 32 bytes long, and 1742
RaptureSoundClip records. Of the hit words `s32(D+0x10)`, 2100 are negative
and the rest are 1 (11 records), 3 (12), 5 (777), 6 (385), 7 (558) and 8
(31). The most common negative records carry `u32(D+0x14..D+0x1C)` = `3, 1,
3` (814), `3, 1, 5` (529), `3, 1, 4` (302), `3, 1, 2` (192) and `8, 1, 6`
(54).

## Slot 1 `0x00829040`

STATIC. Slot 1 loads D from clip `+0x14` (`0x00829070`) and binds the actor
of ordinal `u8(D+3)` (`0x0082907A`). A CharaActor is used directly. Another
actor is cast as a weapon actor and its owner at `+0x238` is used
(`0x008290B8..0x008290E6`). It then branches on the hit word
`h = s32(D+0x10)` (`0x0082911E`).

Negative `h`, with `g = u32(D+0x14)`, `r = u32(D+0x18)` and
`c = u32(D+0x1C)`:

- group `g - 1`, or 6 when `g` is `0x14` (`0x00829129..0x0082913C`)
- index `c - 1 + 10 * (r - 1)` (`LEA` pair at `0x00829142..0x00829146`)
- float 1.0 (`0x00F54F70`), or the result of `0x00828E80` when `g, r, c` is
  `4, 1, 5` (`0x0082913F..0x0082915D`)
- one call `0x0065B1E0(group, index, -1, 0, float)` at `0x00829171`

`h` at or above 0: the actor's battle result object (`0x00A20070`, member
`+0xF0`, virtual `+0x6C`) must be non-null with dword `+4` equal to
`0x00766678` (`0x0082917B..0x0082918A`). For each row index below the u16
count `0x007A0FE0` returns (result `+0x30`):

- `0x007A1140` returns bits 2 to 5 of the row dword at result
  `+0x40 + 0x14 * i`. The byte table `0x00829288` and jump table `0x00829270`
  give a column: 1 gives 0, 2 and 5 give 1, 3 gives 2, 4 gives 3, 15 gives
  4, and every other value skips the row (`0x008291B4..0x008291E8`)
- `0x007A1080` returns bits 27 to 30 of the same dword. 1 and 7 give group 7
  and 2 gives group 8. Every other value skips the row
  (`0x008291FC..0x0082920E`, `0x0082925F..0x0082926A`)
- the call is `0x0065B1E0(group, h - 1 + 10 * column, -1, 0, 1.0)` at
  `0x0082921F`

The row dword is the effect id E of the
[row packing](../actor/attack-scheduler-selection.md#row-packing-0x007b6400),
whose bits 27 to 30 are the kind.

## Actor entry `0x0065B1E0`

STATIC. `0x0065B1E0(group, index, element, flag, float)` returns without a
play unless `0x007CCC20(1)` is true, the actor is the one at
`[actor+0x118]+0xD0`, or `0x007A40C0(actor+0x590)` is true
(`0x0065B1E5..0x0065B209`).

Group 7 calls `0x00847110(0, index)` and group 8 `0x00847110(1, index)` on
the CharaWeaponController at `actor+0x1960` (`0x0065B219..0x0065B240`). A
true answer ends the call. A false answer replaces group 7 with 4 and group
8 with 5. Every group that remains reaches
`0x0084FF50(group, index, element, (flag != 0) - 1, float)` on
`actor+0x21D4` at `0x0065B271`.

## CharaSoundController table

STATIC. `0x0084FF50` returns without a play when `0x0065B1A0` rejects the
actor at controller `+0x18`. It reads the resource id at controller
`+0x1C + 4 * (PLAY[group] + index)` (`0x0084FF82..0x0084FF8F`) and plays
nothing for id 0. The play then takes the actor position through virtual
`+0x88`.

The resource callback `0x00850480` copies a loaded table of load group `L`
to `+0x1C + 4 * LOAD[L]`, at most `LIMIT[L]` bytes
(`0x008504A3..0x008504DF`).

| Table | VA | Dwords |
| --- | --- | --- |
| `PLAY`, by play group 0 to 6 | `0x0103F900` | `0`, `0x14`, `0x1E`, `0x55`, `0x5F`, `0xA5`, `0xEB` |
| `LOAD`, by load group 0 to 4 | `0x0103F91C` | `0`, `0x55`, `0x14`, `0x1E`, `0xEB` |
| `LIMIT` in bytes, by load group | `0x0103F930` | `0x3AC`, `0x258`, `0x28`, `0xDC`, `0x28` |

INFERRED. The table holds `0xF5` ids, the end of load group 4
(`0xEB + 0x28 / 4`). Play groups 7 and 8 never index it, because the actor
entry turns them into 4 and 5 first.

## Weapon sound table

STATIC. `0x00847110(hand, index)` runs `0x0065B1A0` on `[this+0x7C4]`,
requires `hand` below 9 and a weapon at `[this + 0x50 + 0xD0 * hand + 8]`,
and calls `0x008D5AE0(index, hand == 0)` with ECX `weapon+0x19C`
(`0x00847113..0x0084714F`). Without a weapon it returns false.

The RTTI catalog gives `Application::Scene::Actor::Chara::WeaponSoundController`
the vtable at RVA `0xc625ec` (VA `0x010625EC`), slot 1 `0x008D5B50`, and
`CharaWeaponController` the vtable at RVA `0xc3ee4c`.

`0x008D5AE0(index, first)`:

- returns false unless `0x008A51E0` accepts the weapon at `+4` and the
  resource at `+8` returns an id array through virtual `+0x18`
- adds half the count `u32(+0xC)` to the index when `first` is false
  (`0x008D5B10..0x008D5B15`)
- when the index is below the count, plays the id through
  `0x007CF100(0, id, 1.0, 0, 1, 1.0)` on the weapon at `0x008D5B35`
- returns true, also for an index past the count

Slot 1 `0x008D5B50` stores the resource size through virtual `+0x28`
shifted right by 2 at `+0xC` (`0x008D5B6A..0x008D5B77`). `0x008DB650` builds
the weapon request from the strings `equ/e%03d/%s` (`0x01064C98`), `_snd/`
(`0x01064CA8`) and `%04d` (`0x01064CB0`).

## Character table sources

STATIC. The gear routine `0x006B7A40` formats
`/client/chara/%s%03d/equ/e%03d/%s_snd/%04d` (`0x00FD2D50`, pushed at
`0x006B7E79`) and calls `0x00850670` on `chara+0x21D4` at `0x006B7F47`. The
number after the directory prefix is 1 when visual `+0x54` is 0
(`0x006B7E5C..0x006B7E68`). The load group is 1 when visual `+0x54` is 0 and
0 otherwise (`0x006B7F25..0x006B7F3F`). When the low three bits of visual
`+0x48` are nonzero (`0x006B7D83..0x006B7D89`), it formats
`/client/chara/%s%03d/equ/e%03d/%s_snd/8%03d` (`0x00FD2D1C`, `0x006B7D9F`)
and probes a `.le.sri` name (`0x00FD2D48`) before the request.

The voice request at `0x00665F24..0x00665F3C` passes load group 3 and
resource id `0x27D8FFFF + [actor+0x13DC]` to `0x008506C0` on
`actor+0x21D4`, only for a nonzero `[actor+0x13DC]`.

INFERRED. A player model with visual `+0x54` 0 reads `pc/c001` tables into
load group 1, and other models read their own directory into load group 0.

## Install schedulers

MEASURED, with the walk of the method section:

| Scheduler | Action sound records (start frame, words `h, g, r, c`) |
| --- | --- |
| `client/chara/pc/c001/act/clw_clw/atk/base/0001` `atk10_1_1` | 4: `-1, 3, 1, 3`, 8: `5, 0, 0, 0` |
| same bank `atk11_1_1` | 9: `-1, 3, 1, 4`, 14: `6, 0, 0, 0` |
| same bank `atk12_1_1` | 7: `-1, 3, 1, 5`, 12: `7, 0, 0, 0` |
| `client/chara/pc/c001/act/clw_clw/bid/base/0001` `dead` | 10: `-1, 3, 1, 2` |
| `client/chara/mon/m502/act/emp_emp/btl/base/0001` `atk10_1_1` | 11: `-1, 3, 1, 3`, 12: `5, 0, 0, 0` |

INFERRED. A negative record `3, 1, k` plays group 2 index `k - 1`, and a hit
word `h` plays index `h - 1` of the hit column, from the slot 1 arithmetic.

## Open

- `0x00828E80`, the float of the `4, 1, 5` record.
- The gates `0x007CCC20`, `0x007A40C0`, `0x0065B1A0` and `0x008A51E0`.
- The number `0x008DB650` formats for `%04d` and the `.le.sri` probe of
  `0x006B7A40`.
- Placement and attenuation of the actor position in the sound driver.
