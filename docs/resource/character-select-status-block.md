# Character select status block

The character select phase fills the status block of
`system/bootup/CharacterSelection.form` for the selected roster slot. This
page records the control handles of the phase object, the writer
`0x00884CB0`, the static tables it reads, and the select scene resources
BootupManager requests. The markup of the form and the English rows it
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

INFERRED. The select background is the effect pair of `0x7FDB0000`, not a
zone layout, and slot i stands at 1.5 * i plus the mode offset.

## Open

- `0x006A9C90`, `0x006AAF50` and the meaning of the `0x3A` mask.
- The fields of town records `+0x0C` and `+0x10`.
- The actor builder that takes the roster appearance and the update slots
  of the character select camera contexts.
