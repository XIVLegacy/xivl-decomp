# Character select status block

The character select phase fills the status block of
`system/bootup/CharacterSelection.form` for the selected roster slot. This
page records the control handles of the phase object, the writer
`0x00884CB0`, the static tables it reads, and the select scene resources
BootupManager requests, the creation view states and camera context, the
`boot_charaParts` loader, the appearance window table and the bootup
background. The markup of the form and the English rows it
names are in `xivl-client-data:docs/boot-select-and-error-rows.md`. The
[`_zoneParam` boot consumer](../actor/zone-initialization-inputs.md#_zoneparam-boot-consumer)
section owns the zone cache the writer reads, and the
[bootup phase owners](bootup-phase-owners.md) page owns the phase factory.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
base `0x00400000`. All native addresses are VA.

Producing tools are `ghidra` CLI driving Ghidra `12.1.4` cross-reference
and string queries, Capstone `5.0.7` x86-32 disassembly over the
file-backed PE sections, and reads of those sections for strings and
tables. Everything native here is static. The install measurement walks the
RES directories of `xivl-tools:docs/formats/sedb-res.md` with
`tools/vfx_qix_resource.py`.

The phase object is P. Its `+0x220` is the BootupManager B and its
`+0x288` is the selected slot index.

## Control handles

STATIC. `0x008856E0` looks the controls up by name and stores each handle
in P.

| Offset | Control |
| --- | --- |
| `+0x224` | `Label_Menu_Exp` |
| `+0x228` | `IconControl_GuardianIcon` |
| `+0x22C` | `IconControl_SkillCategory` |
| `+0x234` | `TextBlock_PlayerName` |
| `+0x238` | `TextBlock_PlayerName_2` |
| `+0x23C` | `TextBlock_Race` |
| `+0x240` | `TextBlock_TRIBE` |
| `+0x244` | `TextBlock_BirthDay` |
| `+0x248` | `TextBlock_Guardian` |
| `+0x24C` | `TextBlock_Skill` |
| `+0x250` | `TextBlock_SkillRank` |
| `+0x254` | `TextBlock_PhysicalLevel` |
| `+0x258` | `TextBlock_World` |
| `+0x25C` | `TextBlock_Location` |
| `+0x260` | `Border_City` |
| `+0x264` | `Grid_BirthDay` |
| `+0x268` | `Grid_Skil` |
| `+0x26C` | `Grid_World` |
| `+0x270` | `Grid_Location` |

## Status writer `0x00884CB0`

STATIC. `ghidra x-ref to 0x00884cb0` lists the callers `0x008854C0` and
`0x0088553B`. The writer returns at once unless `0x006A9C90(P+0x288)` on B
is true (`0x00884CFF`). The roster entry is `0x006B3D40(index)` with
stride `0x2E0`
(`xivl-client-structs:docs/login-state-boundaries.md`, Roster vector
consumers).

- The name: `0x006A9230` splits roster entry `+0x10` into two buffers, and
  the Text setter `0x009556A0` writes them to `TextBlock_PlayerName`
  (`0x00884E4F`) and `TextBlock_PlayerName_2` (`0x00884E72`).
- The record: entry `+0x40` goes through `0x005AC190` and `0x005A6A90`
  into a ClientSelectDataN object. Its getters are `0x005A0DA0 + 0x20 * k`,
  each pushing its GAM id, read against the ClientSelectDataN registry of
  [gam_registry.h](../../include/net/gam_registry.h): `0x005A0DC0`
  mainSkill (103), `0x005A0DE0` mainSkillLevel (104), `0x005A0E00`
  currentJob (105), `0x005A0E20` physicalLevel (106), `0x005A0E40` tribe
  (107), `0x005A0EA0` guardian (110), `0x005A0EC0` birthdayMonth (111),
  `0x005A0EE0` birthdayDay (112) and `0x005A0F40` initialTown (116).
- The flag test: when `0x006AAF50(index)` on B has any bit of `0x3A` set,
  the branch at `0x00885392` writes Visibility Collapsed to `Border_City`,
  `Grid_BirthDay`, `Grid_Skil`, `Grid_World` and `Grid_Location` and skips
  the rest. `0x006AAF50` was not decoded.

The text writers each end in the Text setter `0x009556A0`: `0x006A4760`
formats `@%d`, `0x006A4810` `@%d/i%d`, `0x006A48D0` `@%d/i%d/i%d/i%d`, and
`0x006A4BB0` reaches `0x006A4A80`, which builds `@%d` followed by `/s%s` or
an integer part per argument. Visibility goes through `0x0093C2A0`, with
Collapsed 0 and Visible 2 as on the
[damage plate numeric presentation](damage-plate-numeric-presentation.md)
page. IconDatas goes through `0x0096A010`
([damage plate form rules](damage-plate-form-rules.md)) and SqwtStyle
through `0x0092F4C0`
([number input digit grid](sqwt-number-input-digit-grid.md)). The error
calls are `0x006A4340(code)` and
`0x006A43A0(code, a, b)`.

Writes of the unflagged branch, in order:

| Site | Control | Value |
| --- | --- | --- |
| `0x00884EA4..0x00884EEC` | `Border_City`, `Grid_BirthDay`, `Grid_Skil`, `Grid_World` | Visible (2) |
| `0x00884F03` | `Grid_Location` | Collapsed (0) |
| `0x00884FBA` | `TextBlock_Race` | `@row`, row = race record `+0xD0` for the race of the tribe record |
| `0x00884FCC` | `TextBlock_TRIBE` | `@row`, row = tribe record `+0x20` |
| `0x00885053` | `TextBlock_BirthDay` | `@100240` with month and day as integer arguments, when the month is 1 to 12 and the day 1 to 32, else error 15000 (`0x0088505A`) |
| `0x00885099` | `TextBlock_Guardian` | `@row`, row = `0x006A9A20(guardian - 1)`, when guardian is above 0, else error 15000 |
| `0x008850F3` | `TextBlock_Skill` | `@231/i{mainSkill}/i{mainSkillLevel}/i{currentJob}` |
| `0x0088513F` | `TextBlock_PhysicalLevel` | `@220/i{physicalLevel}` |
| `0x00885188` | `TextBlock_World` | the text at roster entry `+0x30` |
| `0x008851CF` | `TextBlock_Location` | `@204/i{place}` when the cache `B+0x3D1C+4*index` is nonzero |
| `0x008851E8` | `Grid_Location` | Visible (2) when that cache is nonzero, else Collapsed (0) |
| `0x0088524F` | `IconControl_GuardianIcon` | `%d` of `[0x0105295C + 4 * guardian]`, when guardian is above 0 |
| `0x008852BA` | `IconControl_SkillCategory` | `%d` of `0x00870210(currentJob)` when currentJob is above 0, else of `0x00870210(mainSkill)`, skipped when the result is negative |
| `0x00885338` | `Border_City` | SqwtStyle `[0x012D69E4 + 4 * i]` for the first i below 3 whose town record key equals initialTown |

When initialTown is negative the writer reports error 15001 with argument
1006, and when no town record matches it reports 15001 with 1005
(`0x0088534E..0x00885366`). mainSkillLevel and physicalLevel are read as
signed words and gain `0x80` when negative (`0x008850B5..0x008850BC`,
`0x00885122..0x00885130`). The writer never references `+0x250`, so it
does not write `TextBlock_SkillRank`.

## Static tables

STATIC, read from the file-backed `.data` section.

Tribe records, `0x012BEAF0`, 15 entries of `0x2C` bytes. The lookup
`0x006A9940` matches the key at `+0x04` and returns entry 0 for an
unlisted key.

| Key | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Index `+0x00` | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |
| Model `+0x08` | 1 | 2 | 9 | 3 | 4 | 3 | 4 | 5 | 6 | 5 | 6 | 8 | 8 | 7 | 7 |
| Race `+0x10` | 0 | 0 | 0 | 1 | 1 | 1 | 1 | 2 | 2 | 2 | 2 | 3 | 3 | 4 | 4 |
| Row `+0x20` | 100606 | 100607 | 100608 | 100609 | 100610 | 100611 | 100612 | 100613 | 100614 | 100615 | 100616 | 100617 | 100618 | 100619 | 100620 |

Race records, `0x012BE6A0`, 5 entries of `0xDC` bytes. The lookup
`0x006A9830` matches the key at `+0x00`, copies the record and returns
false for an unlisted key. Keys 0 to 4 hold the rows 100601 to 100605 at
`+0xD0`.

Guardian rows: `0x006A9A20` builds the twelve rows 100838 to 100849 on its
stack and returns the one at its index.

Guardian icons, `0x0105295C`, indexed by guardian: 26721, 347, 348, 349,
350, 351, 352, 353, 354, 355, 356, 357, 358.

Skill icons: `0x00870210` fills the map at `0x0134FF8C` on first use with
3 to 203, 4 to 204, 8 to 205, 2 to 206, 7 to 207, 10 to 208, 30 to 209, 34
to 210, 33 to 211, 29 to 212, 32 to 213, 36 to 214, 35 to 215, 31 to 216,
39 to 217, 40 to 218, 41 to 219, 22 to 233, 23 to 234, 15 to 940, 16 to
939, 17 to 941, 18 to 943, 19 to 942, 26 to 945 and 27 to 944, then
returns the mapped value through `0x00871D00` (`0x008704F5`).

INFERRED. `0x00871D00` inserts a missing key with 0, so an unlisted id
yields icon 0.

Town records, `0x012BEFBC`, 3 entries of `0x14` bytes, read by
`0x006A9A90(i)`:

| i | Key `+0x00` | `+0x04` | `+0x08` | `+0x0C` | `+0x10` | Banner style `0x012D69E4` |
| ---: | ---: | ---: | ---: | --- | --- | --- |
| 0 | 1 | 100621 | 101501 | `0x33` | `0x07FE0046` | `Border_BannerLimsaLominsa` |
| 1 | 2 | 100622 | 101502 | `0x34` | `0x07FE0046` | `Border_BannerGridania` |
| 2 | 3 | 100623 | 101503 | `0x36` | `0x07FE0046` | `Border_BannerUldah` |

## Select scene requests

STATIC. In the BootupManager update, `0x006B0B05` passes the resource id
`0x7FDB0000` to `0x00686E10` on the object at `+0x28`. `0x006B0B52` passes
the second entry of the pointer table `0x012BDF10` (`Scheduler`,
`bg_effect`, `select`) to `0x00686E30`, and `0x006B0B83` passes
`wsw_rtas1f` (`0x00FD23D0`) to the same method.

MEASURED. `data/7F/DB/00/00.DAT` is 1081572 bytes with SHA-256
`dc870878c6f8a7d42404722f632c0f78489366d25f2f59b478a4e20f9ed764b9`. It is
a PWIB whose top RES holds the RES `bg\sch_effect_data\bg_effect` (22236
bytes, with the veff `bg\bin\bg_effect`), the RES
`select\veff\sch_effect_data\select` (84758 bytes, with the veff
`bin\select`), `RESOURCE_TYPE` and `RESOURCE_ID`. The bank holds 2 veff, 9
vtex and 7 vmdl entries and no `SEDBSCB`.

STATIC. `0x006AD530` places the roster actors. It reads the float 1.5 at
`0x012BDF20` (`0x006AE1C0`, `0x006AE309`) with a per-mode float at
`+0x194 + 4 * mode` (`0x006AE1AB`), uses the floats -10.0 (`0x00FB7DD4`)
and 1.0 (`0x00F54F70`), and passes the motion name at `0x012BE69C`
(`cbnm_id0`) to `0x00585BD0` (`0x006AE2AE`) with a timer from
`0x00459E00` modulo 13000 plus 2000 (`0x006AE28D..0x006AE29F`).

For modes 0, 1 and 9 the last arm of each roster entry posts actor message
`0x25` with the 16 bytes (x, 0, 0, 1.0) through `0x004D7980`
(`0x006AE2BC..0x006AE38B`), x being the slot x of the
[row focus](#row-focus-and-select-camera) section. No rotation is written
there.

INFERRED. The select background is the effect pair of `0x7FDB0000`, not a
zone layout, and slot i stands at 1.5 * i plus the mode offset.

### Effect request object

STATIC. The select scene requests run on the object at B `+0x5C`, which
the BootupManager update reaches as ESI `+0x28`. Its request methods are
non-virtual and post through `0x004D7980`.

| Method | Effect |
| --- | --- |
| `0x00686E10(id)` | clears byte `+0x94` and posts message 3 with the 4-byte id |
| `0x00686E30(name)` | clears byte `+0x95` and posts message 4 with 16 bytes of the name |
| `0x00686E50(name, id, a, force)` | with `force` 0 and `id` equal to the dword `+0x98`, returns without a post. Otherwise stores `id` at `+0x98` and posts message 5 with `0x18` bytes: the id, the byte `a` at `+4` and 16 bytes of the name at `+5` |
| `0x00686EC0`, `0x00686ED0` | post messages 6 and 8 with no data |
| `0x00686EE0` | returns byte `+0x94` |

The row focus passes `force` 0, so a second focus on the actor of the last
request posts nothing. The receiver of messages 3, 4 and 5 is not traced.

INFERRED. The object is an `EffectDebugElement`. The constructor
`0x00686FB0`, whose only caller `0x00533B14` allocates `0x120` bytes, stores
the vtable `0x00FC4538` with RTTI
`.?AVEffectDebugElement@Effect@Element@Main@Application@@` at `0x0068701F`,
and the request methods sit beside it and use fields below `0x120`. A
scheduler name such as `bg_effect` or `select` names the RES of
`0x7FDB0000` whose path ends with it, and the id of message 5 is the
focused actor's handle.

### Idle motions

STATIC. The BootupManager update `0x006B0220` runs with ESI = B `+0x34`.

- For each roster actor i that is not null and is not the focused actor
  (`0x006B0444..0x006B0490`), when the timer `ESI+0x3EAC + 4 * i`
  (B `+0x3EE0`) is not negative it subtracts the elapsed time. When the
  result goes negative it starts the motion named at `0x012BE0D4`
  (`cbem_make_id0`) through `0x00585BD0`. The timer is not rearmed here.
- When more than 8000 ms (`0x1F40`) have passed since `ESI+0x15C` by
  `GetTickCount` (import cell `0x00F3E1BC`), and `ESI+0x3E60` equals 3,
  and the mode `ESI+0x144` is 2 to 7 or 9 with a focused slot, it plays
  one of `avert1` to `avert4` (pointer table `0x012BEFA4`) on the focused
  actor through `0x0058C720`, the index being a generator value
  (`0x00459DC0`, `0x00459E00`) modulo `[0x012BDFC4]`, which reads 4
  (`0x006B04A6..0x006B0591`). Once the 8000 ms have passed it stores the
  tick at `ESI+0x15C` whether it played or not (`0x006B05D2..0x006B05DE`).

STATIC. `0x005854D0` posts message `0x2D` with no data, and `0x00587C10`
wraps an angle with the doubles pi (`0x00F62F48`) and 2 pi (`0x00F62FE0`).

INFERRED. With the row focus, an unfocused actor switches to
`cbem_make_id0` once its timer runs out and starts `cbnm_id0` again when
it is focused, since the row focus starts `cbnm_id0` on a focused slot
whose timer is negative. The row focus turns the previous actor rather
than hiding it.

MEASURED. A plain byte search of the install's `client` and `data` trees
finds `cbem_make_id0` in the nine files `client/chara/pc/c001/skl/9998` to
`c009/skl/9998`, and `avert1` to `avert4`, bare and with the prefixes
`cbxf_` and `cfxf_`, in those files and in `kao/<face>/fac_mdl/9998` files.
Which of those names the motion post resolves is not traced.

MEASURED. Each of the nine `skl/9998` files starts with `PWIB`, and a
string scan of each finds the same 13 names:
`cbem_make_id0`, `cbxf_avert1` to `cbxf_avert4`, `cbxf_voice1`,
`cbxf_voice2`, `avert1` to `avert4`, `voice1` and `voice2`.
`client/chara/pc/c010` has no `skl/9998`. `c001/kao/f001/fac_mdl/9998`
names `cfxf_avert1` to `cfxf_avert4`, `cfxf_voice1` and `cfxf_voice2`.
INFERRED. The bare `avert<n>` names that `0x0058C720` takes from
`0x012BEFA4` are entries of this bank, and an avert has a body part
(`cbxf_`) and a face part (`cfxf_`). The record layout of the bank was not
decoded here.

### View state `B+0x3E94`

STATIC. `0x006AAC60(state)`, thiscall on B:

- state -1 stores -1 at `B+0x3E94` and calls `0x0055F450` on the camera at
  `B+0x64` (`0x006AAC66..0x006AAC76`)
- state 2 calls `0x0055C980`, state 3 `0x0055C970` and state 4 `0x0055C990`
  on the camera at `B+0x64` (`0x006AAC7B..0x006AACA3`)
- it stores the state at `B+0x3E94` (`0x006AACAB`). State 3 sets bit 2 of
  the byte `B+0x1CA0`, and any other state clears it unless the dword
  `B+0x1C98` is 15 or 16 (`0x006AACA8..0x006AACCF`)

The `ESI+0x3E60` that the avert arm of the update compares with 3 is
`B+0x3E94`, since ESI is B `+0x34` there.

STATIC. Every caller passes the BootupManager as `[P+0x224]` of its own
object. State 3 comes from `0x00864417`, `0x00868165`, `0x00868377`,
`0x0086847C`, `0x008684BC` and `0x008686F5`, state 2 from `0x0085E8D1`,
`0x0085EF8E`, `0x0085F02E` and `0x0085F12E`.

STATIC. By the [vtable slot catalog](../../config/ffxivgame.vtable_slots.jsonl)
the callers are methods of the `CharacterCreation` phases:

| Caller | Owner and slot | State |
| --- | --- | ---: |
| `0x00868120` | `AppearancePhaseFace` slot 6 | 3 |
| `0x0085EF80` | `AppearancePhaseFace` slot 7 | 2 |
| `0x008686B0` | `AppearancePhaseHair` slot 6 | 3 |
| `0x0085F120` | `AppearancePhaseHair` slot 7 | 2 |
| `0x00868330` | `AppearancePhaseFeature` slot 6 | 3 |
| `0x00864390` | `AppearancePhaseFeature` slot 15 | 3 |
| `0x0085F020` | `AppearancePhaseFeature` slot 7 | 2 |
| `0x00868450`, `0x00868490` | referenced as data from `0x0086B540`, `AppearancePhaseFeature` slot 4 | 3 |
| `0x0085E8B0` | `AppearancePhase` slot 20 | 2 |

INFERRED. The Hair, Face and Feature phases hold view state 3 and return
to state 2 from their slot 7 methods, so the averts that the update gates on
state 3 play only in those phases.

### Creation camera context

STATIC. The camera command receiver `0x0061A6C0` dispatches its command
through the table `0x0061AE48` (`0x0061A6D7..0x0061A6E8`). Entries 28, 29 and
30 send ids `0x1C`, `0x1D` and `0x1E` to `0x0061AADC`, `0x0061AB10` and
`0x0061AB44`. Each finds context key 9 in the registry at `+0x1B0` through
`0x006222B0` and calls `0x007F1170`, `0x007F12B0` or `0x007F13F0` on it.

STATIC. The context constructor arm at `0x007F2700` writes vtable
`0x00FF0F74`, the `CameraActorContextCharaCreate` vtable of the catalog, and
stores 3.0 (`0x00FA7AD8`) at `+0x10`, 5.0 (`0x00FA7B20`) at `+0x14`, 1.0
(`0x00F54F70`) at `+0x1C` and 8.0 (`0x00FBAC18`) at `+0x20`.

STATIC. The handlers write the dword `+0x50` and the float `+0x14`:

| Handler | Camera id | View state | `+0x14` | `+0x50` |
| --- | --- | ---: | --- | ---: |
| `0x007F1170` | `0x1C` | 3 | `+0x1C` | 1 |
| `0x007F12B0` | `0x1D` | 2 | `+0x20` | 0 |
| `0x007F13F0` | `0x1E` | 4 | 2.0 (`0x00F62F64`) | 2 |

Each also stores 0 at `+0x54` and writes `+0x18` and `+0x0C`.

STATIC. The update, slot 1 `0x007F27B0`, clamps `+0x14` to the range 1.0 to
5.0 (`0x007F28A1..0x007F28CE`) after adding the scaled pointer words
`+0x1BE` and `+0x1C2` of the camera. It then switches on `+0x50`: 0 calls
`0x00664100` with the float at `0x00F62F60`, 1 calls `0x00660090` and 2 calls
`0x006601D0` (`0x007F28D3..0x007F2912`).

INFERRED. `+0x14` is the distance goal, so state 3 closes to 1.0 and state 2
draws back to the 5.0 limit, the stored 8.0 reading as 5.0 after the clamp.
The joints the three look-at helpers read and the sender of state 4 were not
traced.

### Facing setter `0x00587C10`

STATIC. `0x00587C10(angle, ...)`, thiscall on an actor element, takes the
current facing as the float at `+0xAC`. When the pointer at `+0x98` is set
it adds the value `0x004DD8B0(1)` returns and wraps the sum to one turn with
the binary64 pi at `0x00F62F48` and two pi at `0x00F62FE0`
(`0x00587C26..0x00587C8F`). When the first argument differs from that
facing, or the byte `+0x264` or `+0x265` is nonzero, it stores the argument
at `+0xAC` (`0x00587C99..0x00587CC9`).

INFERRED. The row focus call `0x00587C10(0.0, 2, 0, 1)` turns the previously
focused actor back to facing 0. What turns a select actor before that was
not traced.

### Name split `0x006A9230`

STATIC. `0x006A9230(name, first, second)` copies the bytes of `name` into
`first` until a space, at most `0x20` bytes. At a space it writes a zero to
`first` and copies the 16 bytes after the space into `second` with two
8-byte moves. With no space in the first `0x20` bytes it returns with
`first` unterminated and `second` untouched (`0x006A9230..0x006A927C`). The
[status writer](#status-writer-0x00884cb0) sets `first` on
`TextBlock_PlayerName` and `second` on `TextBlock_PlayerName_2`.

## Row focus and select camera

STATIC. `0x006AA990(mode, slot)` is the BootupManager row focus, called at
`0x006B0BC0`. B is the BootupManager in ECX.

- It takes the actor array of the mode from `0x006A8EA0` and returns when
  the slot's actor is null and the mode is not 1. It stores the mode at
  `B+0x178`.
- For a mode other than 5, when the slot differs from `B+0x1E8 + 4 * mode`
  it hides the previous actor (`0x005854D0`, then
  `0x00587C10(0.0, 2, 0, 1)`), then stores the slot there
  (`0x006AA9C3..0x006AA9FB`).
- For a slot other than -1 it starts motion `cbnm_id0` (`0x012BE69C`)
  through `0x00585BD0` when the timer `B+0x3EE0 + 4 * slot` is negative,
  and then sets that timer to `0x00459E00() % 13000 + 2000` on the
  generator at `B+0x417C` (`0x006AAA07..0x006AAA3A`).
- The byte table `0x006AAC4C` (0, 0, 0, 0, 0, 1, 0, 0, 2, 0) and the
  arms `0x006AAC40` send modes 0 to 4, 6, 7 and 9 to `0x006AAA66`, mode 5
  to `0x006AAB7C` and mode 8 to `0x006AAB8B`. A mode above 9 also takes
  `0x006AAB8B`.

The `0x006AAA66` arm (`0x006AAA66..0x006AAB6B`):

- `x = 0x006A9470(mode, slot)`, also stored at `B+0x198 + 4 * mode`.
- `t = B+0x210 + 4 * slot`, the camera index.
- `0x0055C8E0(first, second, 0.02, 5.0)` on the camera at `B+0x64`, with
  `first = (x, H[t], D[t], 1.0)`, `second = (x, H[t], 0.0, 1.0)` and the
  floats at `0x012BDF24` and `0x012BDF28`, then `0x0055F250` on the same
  camera.
- When the slot's actor exists: `0x0055C290(1)`, `0x0055C2B0(1)`,
  `0x0055C2D0(0x004D6750(0))` and `0x0055C310` on `B+0x60`,
  `0x00686E50("select", id, 1, 0)` on `B+0x5C` (string pointer
  `0x012BDF18`, `0x006ADD41..0x006ADD5B`, id being the dword `+0x88` of
  the actor that `0x004D6750` returns), `0x004D7660` on the actor, and the
  slot is stored at `B+0x1C0 + 4 * mode`.

When the slot changes, the `0x006AAB8B` path calls `0x00588280(f, 1)` on the previous slot's
actor and, for a slot with an actor, `0x00588280(1, 1)` on it, with no
camera goal (`0x006AAB8B..0x006AAC2F`). The mode 5 arm stores mode 5,
sets f to 0 and joins it. Every other route reaches it with f 1.

`0x006A9470(mode, slot)` returns `slot * 1.5 + offset` with 1.5 at
`0x012BDF20`. The offset is 0 for modes 0, 1 and 9 and for a mode of 0 or
below, and `B+0x194 + 4 * mode` for any other mode. Mode 5 uses slot 0.
`0x006A9440(mode)` returns the offset alone (`0x006A9440..0x006A94BC`).

Several writers in `0x006AD530` store the dword at `+0x00` of the tribe
record that `0x006A9940` returns into `B+0x210 + 4 * slot`
(`0x006AD704`, `0x006ADC66`, `0x006ADE49`, `0x006ADEF5`). Others store
values from `0x006ACBE0` and `0x006A99C0` paths (`0x006ADA94`,
`0x006ADAC9`, `0x006ADB35`), not traced here.

Camera tables, 15 floats each, read from the file-backed `.data` section:

| t | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| D `0x012BDF40` | 3.4 | 3.2 | 3.7 | 3.8 | 3.8 | 3.8 | 3.8 | 2.7 | 2.7 | 2.7 | 2.7 | 3.0 | 3.0 | 4.2 | 4.2 |
| H `0x012BDF7C` | 1.1 | 1.05 | 1.2 | 1.25 | 1.25 | 1.25 | 1.25 | 0.9 | 0.9 | 0.9 | 0.9 | 1.0 | 1.0 | 1.4 | 1.4 |

### Camera goal

STATIC. `0x0055C8E0(first, second, speed1, speed2)` copies the two
16-byte points and the two floats into a 40-byte payload and posts camera
message `0x18` (`0x0055C90E..0x0055C935`). `0x0055F250` posts command
`0x10` with the dword 8, and `0x0055F190(f)` posts command `0x14` with
the float. `0x0055C520(b)` posts command `0x2B` with `b != 0`.

In the camera command receiver `0x0061A6C0` the table `0x0061AE48` sends
id `0x18` to `0x0061A9C3`. That arm finds context key 8 in the registry
at `+0x1B0` through `0x006222B0` and calls `0x007F2690(first, second,
payload +0x20, payload +0x24)` on it (`0x0061A9C3..0x0061AA0E`).
`0x007F2690` stores the points at `+0x30` and `+0x40` and the floats at
`+0x50` and `+0x54` (`0x007F2696..0x007F26B9`). The constructor
`0x0061B5A0` inserts the `CameraActorContextCharaSelect` built by
`0x007F2400` under key 8 into `+0x1B0` (`0x0061B972`, `0x0061BB7E`,
`0x0061BB98`, `0x0061BBA4`).

STATIC. The CharaSelect update is slot 1 of vtable `0x00FF0F24`,
`0x007F24A0(camera, dt)` (`0x007F24A0..0x007F268A`). For each pair,
current `+0x10` toward goal `+0x30` with speed `+0x50`, then current
`+0x20` toward goal `+0x40` with speed `+0x54`:

- `share = dt * 0.3` (double `0x00FB7DE0`), or 1.0 when that exceeds 1.0
  (double `0x00F598A0`).
- `step = (goal - current) * share`.
- When the xyz length of the step is at least `speed * dt`, the step
  becomes its unit vector (`0x0061CFD0`) times `speed * dt`.
- The current point gains the step.

It then writes `+0x10` to camera `+0x330` and `+0x20` to camera `+0x340`.
The [update pipeline](../actor/camera-input-chain.md#update-pipeline-and-command-receiver)
passes `dt` to the context's `+0x04`.

### Boot camera

STATIC. The BootupManager constructor `0x006B3210` sends command `0x14`
with 0.6606713 (`0x012BDF3C`) at `0x006B3890`, command `0x10` with 8 at
`0x006B3928`, the goal `(2.5, 1.1, 20.0, 1.0)` and `(0.0, 1.1, 0.0, 1.0)`
with speeds 0 and 0 at `0x006B3946` (2.5 at `0x00FD25D8`, 1.1 at
`0x012BDF1C`, 20.0 at `0x00FEB194`), and command `0x2B` with 1 at
`0x006B3950`. The BootupManager update sends the same two points with
speeds 0.02 and 5.0 at `0x006B0A23`, after command `0x10` with 8
(`0x006B09A5..0x006B0A23`).

The mode 5 path at `0x006B1580..0x006B1610` sends first
`(x, H[t], 10.0, 1.0)` and second `(x, H[t], 0.0, 1.0)` with speeds 1.0
(`0x012BDF34`) and 5.0 (`0x012BDF38`), 10.0 at `0x012BDF30`. Here x is
`0x006A9440(m)` and `t = B+0x210 + 4 * (B+0x1C0 + 4 * m)`, the camera index
of the slot last focused in mode m (ESI is B + `0x34` in that function).

INFERRED. The first point is the eye and the second the look-at, so the
eye of a focused slot stands D[t] in front of the slot at height H[t] and
looks at the slot at the same height. Command `0x14` writes `+0x404`, the
float the [command table](../actor/camera-input-chain.md#update-pipeline-and-command-receiver)
records for id 20, and reading it as the field of view is INFERRED. With
`dt` in 30 Hz frames, the eye closes 0.3 of the way each frame up to 0.02
yalms a frame.

Reproduce with `ghidra disasm 0x006aa990 -n 200`, `ghidra x-ref to
0x006aa990`, `ghidra disasm 0x006a9440 -n 40`,
`ghidra disasm 0x0055c8e0 -n 20`, `ghidra disasm 0x0061a9c3 -n 30`,
`ghidra disasm 0x007f24a0 -n 160`, `ghidra disasm 0x007f2690 -n 12`,
`ghidra x-ref to 0x007f2400` and `ghidra disasm 0x006b3881 -n 50`, and
read the floats at `0x012BDF1C..0x012BDFB7`, the bytes at `0x006AAC4C`,
the dwords at `0x006AAC40` and the cell `0x00FF0F28`.

## Creation sheet holders

STATIC. The BootupManager constructor `0x006B3210` opens five sheets by
name through `0x004489C0` into holders of B:

| Sheet | Name literal | Holder | Open | Registration slot |
| --- | --- | --- | --- | --- |
| `boot_charaParts` | `0x00FD2384` | `B+0x1CA8` | `0x006B3A2C` | `B+0x1CFC` |
| `boot_charaTemp` | `0x00FD2394` | `B+0x3384` | `0x006B3A44` | `B+0x33D8` |
| `boot_charaLook` | `0x00FD23A4` | `B+0x35C0` | `0x006B3A54` | `B+0x3614` |
| `boot_skillequip` | `0x00FD23B4` | `B+0x3ACC` | `0x006B3A64` | `B+0x3B20` |
| `_zoneParam` | `0x00FD23C4` | `B+0x3CC4` | `0x006B3A74` | |

STATIC. `0x006AF200`, a BootupManager method that the ghidra project has
no function for, registers the first four holders once. When a
registration slot is zero it calls `0x004D74C0` on `[B+4]`, calls virtual
`+4` of the result with the holder address and stores the answer in the
slot (`0x006AF363..0x006AF3F5`, `boot_skillequip` at
`0x006AF3D5..0x006AF3F5`).

The decoded sheet shapes are in
`xivl-client-data:docs/character-creation-sheet-shapes.md`.

### Parts loader

STATIC. The BootupManager update arm at `0x006B19BD..0x006B1A3D` reads the
handle at `ESI+0x1CC8`, which is the `boot_charaParts` registration slot
`B+0x1CFC` because ESI is B `+0x34` there. It advances the row counter
`ESI+0x1B78` once per pass (`0x006B19FE`) and stops at 15
(`0x006B1A3D`). For cells 0 to 23 (`CMP EDI,0x18` at `0x006B1A2D`) it
passes the row, the cell and the cell text to `0x006AECE0` on B
(`0x006B1A07..0x006B1A30`).

STATIC. `0x006AECE0(row, cell, text)` splits the text at commas
(`0x006AED8E`). An entry without `/` (`0x00FD2414`) is read with `_atol`,
or for cells `0x12` to `0x16` through `0x006A8F50`
(`0x006AEE65..0x006AEE88`). An entry with `/` is split at `/`
(`0x006AEF52`) and its last element read with `_atol` plus 1
(`0x006AEFAD..0x006AEFEB`). Each value is appended through `0x006B4FF0` to
the list at `B + 16 * (cell + 24 * row + 0x1B8)` (`0x006AEEA0..0x006AEEB1`).

STATIC. `0x006AA030(cell)` answers the list at
`B + 16 * (cell + 24 * index + 0x1D0)`, `index` being the dword
`0x006A99C0(B+0x1C8, B+0x1CC)` returns (`0x006AA030..0x006AA05E`).

INFERRED. `0x006A8F50` reads a hexadecimal colour, the slash entries store
the count of a list's variations, and `0x006AA030` answers a list of the
current tribe.

### Appearance window table

STATIC. The table at `0x012D1738` holds 18 entries of four dwords up to
`0x012D1858`. `0x0085E7F0(window)` finds the entry whose first dword is the
window and copies all four (`0x0085E7F0..0x0085E833`).

| Window | Dword 1 | Dword 2 | Dword 3 | Window form |
| ---: | ---: | ---: | ---: | --- |
| 0 | 1 | 9 | 0 | `AppearancePhaseSizeWin` |
| 1 | 18 | 10 | 0 | `AppearancePhaseVoiceWin` |
| 2 | 16 | 0 | 18 | `AppearancePhaseSkinColorWin` |
| 3 | 2 | 1 | 0 | `AppearancePhaseHairStyleWin` |
| 4 | 15 | 3 | 19 | `AppearancePhaseHairColorWin` |
| 5 | 3 | 8 | 21 | `AppearancePhaseHairMColorWin` |
| 6 | 4 | 2 | 0 | `AppearancePhaseHairVaWin` |
| 7 | 17 | 4 | 20 | `AppearancePhaseFacePColorWin` |
| 8 | 5 | 5 | 0 | `AppearancePhaseFaceTypeWin` |
| 9 | 8 | 13 | 0 | `AppearancePhaseFaceEyebroWin` |
| 10 | 10 | 12 | 0 | `AppearancePhaseFaceEyeWin` |
| 11 | 9 | 11 | 0 | `AppearancePhaseFacePupilWin` |
| 12 | 11 | 14 | 0 | `AppearancePhaseFaceNoseWin` |
| 13 | 13 | 15 | 0 | `AppearancePhaseFaceMouthWin` |
| 14 | 14 | 17 | 0 | `AppearancePhaseFeatureWin` |
| 15 | 12 | 16 | 0 | `AppearancePhaseFaceContWin` |
| 16 | 6 | 21 | 0 | none |
| 17 | 7 | 22 | 22 | `AppearancePhaseFColorWin`, `AppearancePhaseFTColorWin` |

STATIC. The form column comes from the phase methods that push a form path
and a window id before calling `0x00863970` or `0x00863750`: the path
pushes at `0x0086935D` (1), `0x00869395` (2), `0x00868538` (3),
`0x008685AB` (4), `0x008685E3` (5), `0x00868573` (6), `0x00867FAB` (7),
`0x00867EC8` (8), `0x00867F03` (9), `0x00867F3B` (10), `0x00867F73` (11),
`0x00867FE3` (12), `0x0086801B` (13), `0x00868233` (14), `0x00868053` (15),
`0x008681F8` (17) and `0x0086826B` (17). The Size form push at `0x00869324`
passes EBX as the id, read as 0. No push for window 16 was found among
these.

STATIC. Readers of the entry:

- `0x00863C60` and `0x00863DB0` pass dword 2 to `0x006AA030`
  (`0x00863C72..0x00863C89`, `0x00863DC5..0x00863DDD`). `0x00863CF0` also
  reads the entry (`0x00863D69`).
- `0x00863830` passes dword 3 to `0x006AA030` (`0x00863845..0x0086387A`).
- `0x00867040` reads dword 1 as an index into the vector at `[EBX+4]`
  (`0x008671A6..0x008671FA`).

STATIC. `0x00864120` fills the value at `+0x29C` of each of up to 32 Buttons
at `+0x250 + 4 * index`. Windows 6 and `0x11` store the index. Every other
window stores the entry of its dword 2 list at the index from `0x00863DB0`,
for an index below the count `0x00863CF0` gives (`0x0086414F..0x00864191`).

INFERRED. Dword 2 names the `boot_charaParts` cell that lists a window's
values, dword 3 the colour cell of a colour window, and dword 1 the
`boot_charaLook` cell that holds a template's value for the window. The data
join is in `xivl-client-data:docs/character-creation-sheet-shapes.md`.

## Bootup background

STATIC. The form string `\system\bootup\BootupBg.form` at `0x01052048` has
one reference, `0x008838E8` in the BootupBg constructor `0x008838A0`.
`0x006A9C60` loads `[B+0x1BA8]` and jumps to `0x008839B0`.

STATIC. BootupBg slot 4, `0x008839E0`, binds the Labels named by the pointer
table `0x012D68D8` (`Label_BgLimsaLominsa`, `Label_BgGridania`,
`Label_BgUldah`) and the Borders named by `0x012D68E4`
(`Border_BgLimsaLominsa`, `Border_BgGridania`, `Border_BgUldah`). It divides
two sizes by 1920.0 (binary64 `0x01052118`) and 1200.0 (binary64
`0x01052110`), formats the smaller quotient with `%.4f` and writes the text
to `RenderTransform.ScaleX` and `RenderTransform.ScaleY` of each Border
(`0x00883ACF..0x00883B9F`).

STATIC. `0x008839B0(city, command)` sends the string
`[0x012D68CC + 4 * command]` to the Label at `+0x1B8 + 4 * city` through
`0x006A4ED0`. The table holds `BG_FadeIn`, `BG_FadeOut` and `BG_FadeOut`.

STATIC. The callers of `0x006A9C60` are `0x00860110` (`0x00860153`,
`0x00860167`), slot 21 of `LocationPhase`, and `0x00865C60` (`0x00865C90`,
`0x00865D5E`, `0x00865D9A`), slot 20 of `LocationPhase`.

INFERRED. The city backgrounds fade in and out only on the location step,
in the city order Limsa Lominsa, Gridania, Ul'dah.

Reproduce the creation sections with `ghidra x-ref to 0x006aac60`,
`ghidra disasm 0x0061a6c0 -n 12`, `ghidra memory read 0x0061ae48 160`,
`ghidra disasm 0x0061aadc -n 14`, `ghidra disasm 0x007f2700 -n 30`,
`ghidra disasm 0x007f1170 -n 40` (and `0x007f12b0`, `0x007f13f0`),
`ghidra disasm 0x007f2892 -n 40`, `ghidra disasm 0x006b19bd -n 50`,
`ghidra disasm 0x006aece0 -n 260`, `ghidra disasm 0x006aa030 -n 20`,
`ghidra memory read 0x012d1738 288`, `ghidra x-ref to 0x012d1738`,
`ghidra disasm 0x00864120 -n 80`, `ghidra disasm 0x0086935d -n 2` for each
form push above, `ghidra x-ref to 0x01052048`, `ghidra disasm 0x008839b0 -n 10`,
`ghidra disasm 0x00883ab8 -n 60` and `ghidra x-ref to 0x006a9c60`. Read the
floats `0x00FA7AD8`, `0x00FA7B20`, `0x00F54F70`, `0x00FBAC18`, `0x00F62F64`,
the binary64 values `0x01052110` and `0x01052118`, the strings at
`0x0104A2D8` and the other form pushes, `0x01051FAC..0x01052090` and
`0x00FD2414`, and the owners of the caller slots in the vtable slot catalog.

## Open

- `0x006A9C90`, `0x006AAF50` and the meaning of the `0x3A` mask.
- The fields of town records `+0x0C` and `+0x10`.
- The actor builder that takes the roster appearance, the meaning of actor
  message `0x25`, and the select update slots of the CharaCreate and
  CharaFocus camera contexts.
- The readers of `boot_charaTemp`, `boot_charaLook` and `boot_skillequip`,
  the vector `0x00867040` indexes by dword 1 of the window table, and the
  window that id 16 serves.
- The joints the CharaCreate look-at helpers `0x00664100`, `0x00660090` and
  `0x006601D0` read, and the sender of view state 4.
- The consumer of the BootupBg Label commands and the sizes its init
  divides.
- The resolution of `cbem_make_id0` and `avert1` to `avert4` against the
  `9998` files, the receiver of the effect messages 3, 4 and 5, and the `bg_effect` and
  `select` schedulers of `0x7FDB0000`.
