# Effect caster clips and damage side

A weaponskill effect bank starts its caster motion through a chain of
scheduler clips, and the receiver of a hit picks a damage motion by the side
the attacker stands on. This page records the enter routines of the two
caster scheduler clips, the scheduler chain of three installed `wsc` banks,
the side code of `0x007BF8C0`, the motion names `0x007BBDD0` tries, and the
contents of the installed `dead` schedulers. The
[attack scheduler selection](attack-scheduler-selection.md) page owns the
auto attack scheduler and the reaction name table. The
[packed selector](animation-bank-routing.md#packed-selector) and
[posture source](animation-bank-routing.md#posture-source-and-snapshot)
sections own the bank loads and the `main` search of `0x00844330`. The
[fall progression](locomotion-selection.md#fall-progression-and-return-to-field)
section owns the death phases and their scheduler names.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
base `0x00400000`. All native addresses are VA.

Producing tools are `ghidra` CLI driving Ghidra `12.1.4` disassembly and
cross-reference queries, and reads of the file-backed PE sections for
strings, jump tables and constants. Every lane, offset and constant below was
read from instruction text or section bytes. Install measurements walk the
RES directories of `xivl-tools:docs/formats/sedb-res.md`, nested RES entries
included, and parse each `SEDBSCB` child with `tools/scb_clip_scheduler.py`.
A clip start frame is `u32(D+4)` over 10000 ticks
([clip timing](../resource/cut-scheduler-clip-timing.md#time-unit)), and a
MotionClip end frame is `u32(D+0x18)` over 10000. Everything native here is
static.

## Caster clip enter routines

STATIC. The RTTI catalog
([ffxivgame.rtti.json](../../config/ffxivgame.rtti.json)) gives
`RaptureCasterManagedSchClip` the 42-slot vtable at RVA `0xc12424` (VA
`0x01012424`) and `RaptureCasterSchClip` the one at RVA `0xc117e4` (VA
`0x010117E4`). Slot 1 of the first is `0x00820880` and slot 1 of the second
is `0x0081E3D0`.

`0x00820880` asks the bound actor's object at `+0xF0` for its vfx parameter
(virtual `+0x6C`). A null parameter formats the literal
`[ERROR] RaptureCasterManagedSchClip: None Set VfxParameter` (`0x010119D8`,
pushed at `0x008208C5`). A parameter whose dword `+4` is not `0x00766678`
takes the other exit at `0x00820920`. Otherwise it calls `0x00820680` with
its argument (`0x00820968`).

`0x00820680` loads the clip record D from clip `+0x14` (`0x008206BA`) and
passes `D+0x11` (`LEA ECX,[EBP+0x11]` at `0x008206F2`) as the first argument
of `0x0081FF40` (`0x00820700`).

`0x0081FF40(name, scheduler, out)`:

- binds the actor of ordinal `u8(D+3)` through `0x00A1FFA0` and compares its
  class (virtual `+0x10`) with the descriptor `0x00A1BB80` returns for the
  literal `ProxyActor` (`0x01011AF0`). A different class returns 0
  (`0x0081FF9D..0x0081FFA9`)
- copies the name into a 16-byte buffer (`0x0062E2D0(buffer, name, 0x10)` at
  `0x00820020`) and looks it up with the type word `0x00736362` (`scb`)
  through virtual `+0x10` of the actor's resource object
  (`0x00820025..0x0082003E`)
- on a miss, or without that object, looks the same key up through
  `0x007CB890` on the resource manager of the scheduler
  (`0x00820062..0x0082009C`)

`0x0081E3D0` formats `[ERROR] RaptureCasterSchClip: None Set VfxParameter`
(`0x01010D50`, pushed at `0x0081E6FB`) for a null parameter. It resolves an
actor through `0x0081D890` (`0x0081E4A1`), then branches on the signed byte
`+0xC` of the parameter (`0x0081E4AE`):

- not negative: one start through `0x0081F090` for each entry below the
  count of `0x0080B0F0`, with the entry of `0x0080B020`
  (`0x0081E4B8..0x0081E500`)
- negative: the action targets of the parameter. A zero count of
  `0x007A0FE0` starts once with target -1 (`0x0081E507..0x0081E530`).
  Otherwise each target index passes the presence test `0x007A1070` and a
  kind from `0x007A1080` other than 5 before `0x007A0FF0` reads the target
  and `0x0081F090` starts it. When byte `+0x24` of the target list is set the
  walk starts at index 1 (`0x0081E53E..0x0081E569`)

MEASURED. In every installed record read for this page,
RaptureCasterManagedSchClip holds a NUL-terminated scheduler name at
`D+0x11` and RaptureCasterSchClip one at `D+0x10`, each inside 16 bytes. The
reader of the `D+0x10` name was not located.

## Weaponskill effect banks

MEASURED. `client/vfx/wsc/1027` (214656 bytes) holds three schedulers:

| Scheduler entry | Clips (start frame) |
| --- | --- |
| `system\shoot_ws\main\bin\main` | BindActorClip 0, RaptureCasterManagedSchClip `sht00` 0, RaptureCasterManagedSchClip `ws_main` 0, RaptureActionSelectClip 0, ClipSyncClip 108 |
| `weapon\wgp\uraken01\main\bin\ws_main` | BindActorClip 0, RaptureServerMoveStopClip 0, RaptureStatusLockClip 0, RaptureClientMoveStopClip 0, MotionClip 0 to 65, RaptureSoundClip 26, ActionClip 26, RaptureCasterSchClip `wsw_csla1tar` 34, MotionClip 65 to 110, ClipSyncClip 80 |
| `weapon\wgp\uraken01\wsw_csla1tar\bin\wsw_csla1tar` | BindActorClip, RaptureSoundClip, RaptureActionSelectDamageMccClip, ActionClip, all at 0 |

The bank also holds 5 vtex, 6 vmdl, 2 veff, 2 leaf, 2 vins and 2 `ACB`
entries. Its strings name the motions `cbbm_ws_z_1` and `cbbm_ws_z_2`. Both
are entries of `client/chara/pc/c001/act/clw_clw/wsc/base/0001`, which holds
no scheduler.

`client/vfx/wsc/1028` (351128 bytes) and `client/vfx/wsc/0008` (249076
bytes) have the same `main`. Their `ws_main` starts the target scheduler
`18_0063` or `18_0008` at frame 30, holds two MotionClips at 0 to 65 and 65
to 110, and names `cbbm_ws_t_1`/`_2` or `cbbm_ws_d_1`/`_2`. Those motions are
entries of `clw_clw/wsc/base/0002` and `0003`, which hold no scheduler
either. No scheduler or entry named `sht00` was found in these six files.

INFERRED. The caster motion of these effects lives in `ws_main`, one
RaptureCasterManagedSchClip below the `main` that `0x00844330` finds, and its
motion names resolve in the character bank of the weapon set.

## Damage side `0x007BF8C0`

STATIC. `0x007BF8C0(record, flag, mode)` has one caller, `0x0065E50E` in
`0x0065E4E0`. It returns at once when the receiver's virtual `+0x268` is
nonzero (`0x007BF8EE..0x007BF8F2`). `battle` is true when the receiver's
signed byte `+0xB39` is 2 or `0x51` (`0x007BF8FA..0x007BF910`).

For `mode` below 4 and a source actor `0x007A0150(record+0x74)`:

- `d = source - receiver`, both positions through virtual `+0x88`
  (`0x007BF945..0x007BF972`)
- when `|d|` is at least the float 0.001 at `0x00FB7AB4` (`0x007BF99A`),
  `a = atan2(d.x, d.z) - facing`, with `d.x` and `d.z` loaded at
  `0x007BF9BC` and `0x007BF9C0` for `0x009DAEBA` and the facing the float
  `+4` of the receiver's virtual `+0x94` (`0x007BF9DB`)
- mode 1 adds the binary64 at `0x00F62F48` (pi), mode 2 subtracts the one
  at `0x00F62FF8` (pi/2) and mode 3 adds it (`0x007BF9ED..0x007BFA1A`)
- `0x007BFA1E..0x007BFA64` adds and subtracts the binary64 2pi at
  `0x00F62FE0` until `a` lies in the range above -pi up to pi

| Range of `a` | Code | Locators |
| --- | ---: | --- |
| -pi/4 to pi/4, both included (`0x00FE81B0`, `0x00FE81A8`) | 1 | `0x007BFA66..0x007BFA8A` |
| -3pi/4 (`0x00FE81A0`) up to -pi/4 | 8 | `0x007BFA94..0x007BFAAC` |
| above pi/4 up to the float 3pi/4 at `0x00FE8198` | 2 | `0x007BFAB6..0x007BFACD` |
| anything else | 4 | `0x007BFAD7` |

Without a source actor, with `|d|` below 0.001, or with `mode` at least 4,
the code comes from the signed remainder of `mode` by 4 through the table at
`0x007BFBB4`: 0 gives 1, 1 gives 4, 2 gives 8 and 3 gives 2
(`0x007BFAE1..0x007BFB1E`).

It then walks the u16 row count at `record+0x98` with rows of 0x14 bytes,
the target at `record+0xA0` and the effect id at `record+0xA8`. The first
row whose target is the receiver's `+0xDC` and whose kind `(E >> 27) & 0xF`
is 1, 2 or 7 calls `0x007BBDD0(flag, code, battle)` (`0x007BFB50..0x007BFB99`).

## Damage motion names `0x007BBDD0`

STATIC. `0x007BBDD0` has 13 direct callers, from `0x007BFB99` to
`0x007C0C00`. Each name goes to `0x007BA0E0`, and the first that returns
true ends the list. `battle` selects the first string of each pair below
(`cbbm_`/`cbba_`), and false the second (`cbnm_`/`cbna_`).

When `0x007A3EE0(receiver+0x590)` is true, `+0x1C` of `this` is 0 and `flag`
is nonzero, the code goes through the table at `0x007BC0B8`:

| Code | Names in order | Strings |
| ---: | --- | --- |
| 1 | `dmgm_f`, `add_dmgh_f` | `0x00FE6660`..`0x00FE6688` |
| 2 | `dmgm_r`, `dmgm_f`, `add_dmgh_r`, `add_dmgh_f` | `0x00FE6708`..`0x00FE6768` |
| 4 | `dmgm_b`, `dmgm_f`, `add_dmgh_b`, `add_dmgh_f` | `0x00FE6778`..`0x00FE67D8` |
| 8 | `dmgm_l`, `dmgm_f`, `add_dmgh_l`, `add_dmgh_f` | `0x00FE6698`..`0x00FE66F8` |

The `dmgm` names take the `m` prefix (`cbbm_dmgm_f`) and the `add_dmgh` names
the `a` prefix (`cbba_add_dmgh_f`).

Otherwise the table at `0x007BC0D8` plays the additive names: code 1
`add_dmg_f`, code 2 `add_dmg_r` then `add_dmg_f`, code 4 `add_dmg_b` then
`add_dmg_f`, code 8 `add_dmg_l` then `add_dmg_f`, as `cbba_` or `cbna_`
(strings `0x00FE6580`..`0x00FE6650`). Any other code plays nothing.

INFERRED. Code 1 is the front, 4 the back, 2 and 8 the two flanks, from the
pi offset of mode 1, the pi/2 offsets of modes 2 and 3, and the `_f`, `_b`,
`_r` and `_l` suffixes of the names each code tries.

## Installed `dead` schedulers

MEASURED. No file below `client/chara/pc/c001/act` (995 files) holds a
scheduler entry whose last path component is `dead1` or `dead2`. Twelve hold
one named `dead`. Each `dead` read below holds BindActorClip,
RaptureServerMoveStopClip and RaptureClientMoveStopClip at frame 0 and one
MotionClip from frame 0. The c001 `clw_clw` one also holds a
RaptureCharaActionSoundClip at frame 10.

| Bank | Scheduler entry | MotionClip end frame |
| --- | --- | ---: |
| `pc/c001/act/emp_emp/bid/base/0001` | `sch\pc\c001\emp_emp\s_bid00` / `dead` | 155 |
| `pc/c001/act/clw_clw/bid/base/0001` | `sch\pc\c001\clw_clw\s_bid00` / `dead` | 125 |
| `pc/c001/act/swd_emp/bid/base/0001` | `sch\pc\c001\swd_sld\s_bid00` / `dead` | 120 |
| `pc/c001/act/clb_sld/bid/base/0001` | `chr\sch\pc\c001\swd_sld\dead` | 120 |
| `pc/c001/act/2bw_emp/bid/base/0001` | `sch\pc\c001\2bw_emp\s_bid00` / `dead` | 120 |
| `pc/c003/act/emp_emp/bid/base/0001` | `sch\pc\c003\emp_emp\s_bid00` / `dead` | 155 |
| `mon/m011/act/emp_emp/bid/base/0000` | nested `s_bid00` / `dead` | 100 |
| `mon/m502/act/emp_emp/bid/base/0000` | nested `s_bid00` / `dead` | 97 |

A slash separates a nested RES entry from the scheduler entry inside it.

INFERRED. With no `dead1` and no `dead2`, death phase 1 of
[fall progression](locomotion-selection.md#fall-progression-and-return-to-field)
starts `dead` and stores phase 4 for these banks. Phase 4 ends near the last
frames of the scheduler, before the dead pose of phase 5.

## Open

- The reader of the RaptureCasterSchClip name at `D+0x10`, and what
  `0x0081F090` starts on each target.
- The `mode` and `flag` that `0x0065E4E0` passes to `0x007BF8C0`, and the
  callers of `0x007BBDD0` other than `0x007BF8C0`.
- The scheduler that RaptureActionSelectDamageMccClip of a target scheduler
  starts, and the role of `sht00`.
