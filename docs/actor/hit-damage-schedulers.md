# Hit damage schedulers

The receiver of a hit plays its damage effects through schedulers of the
resident effect bank. This page records the resident id list the client
requests, the damage schedulers that bank holds, the route from the
`normal_dam` scheduler's RaptureActionSelectDamageMccClip to the result
decoder `0x007A1810`, the scheduler names the kind 1 and 2 arm of that
decoder starts, and the element name tables of `0x00855160`. The
[resident effect banks](crafting-presentation.md#resident-effect-banks)
section owns the bank identities, the
[transient result effects](crafting-presentation.md#transient-result-effects)
section owns the crafting arm of the same decoder, and the
[install agreement](attack-scheduler-selection.md#install-agreement) section
owns the auto attack schedulers that start `normal_dam`.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
base `0x00400000`. All native addresses are VA.

Producing tools are `ghidra` CLI driving Ghidra `12.1.4` cross-reference
queries, Capstone `5.0.7` x86-32 disassembly over the file-backed PE
sections, and reads of those sections for strings, jump tables and
constants. Every lane, offset and constant below was read from instruction
text or section bytes. Install measurements walk the RES directories of
`xivl-tools:docs/formats/sedb-res.md` with `walk_bank`, `parse_res` and
`name_table` of `tools/vfx_qix_resource.py`, nested RES entries included,
and parse each `SEDBSCB` child with `tools/scb_clip_scheduler.py`. A clip
start frame is `u32(D+4)` and an ActionClip end is `u32(D+0x18)`, both over
10000 ticks per second
([clip timing](../resource/cut-scheduler-clip-timing.md#time-unit)).
Everything native here is static.

## Resident id list

STATIC. `0x0055FB40` is the vtable cell at `0x00FA4928`. It reads the
index at `this+0x94` and returns when it is -1. Otherwise it takes the
pointer `[0x0126F4B0 + 4 * index]`, walks the zero-terminated u32 list it
points to and passes each id to `0x004D7980` (`0x0055FB60..0x0055FB7C`),
then stores -1 at `+0x94`. The table at `0x0126F4B0` holds one pointer,
`0x00FA4948`, followed by 0. The list at `0x00FA4948` is `0x15D90000`,
`0x15D90002`, `0x15D90003`, 0.

MEASURED. The install holds `data/15/D9/00/00.DAT` and `01.DAT` and no
`02.DAT` or `03.DAT`. So of the three listed ids only `0x15D90000` names an
installed file, and `01.DAT` is not in the list.

## Damage schedulers in the resident bank

MEASURED. `data/15/D9/00/00.DAT` is a PWIB whose top RES (offset `0x10`)
has five entries: the RES `vfx\resident\common_rpe\InGame_Vfx` (4473805
bytes), `chr\cib\cibw\weponindex`, the RES `chr\sch\pc\cmn\btlcmn` (7522
bytes), `RESOURCE_TYPE` and `RESOURCE_ID`. Over the whole bank the walk
finds 248 `SEDBSCB`, 221 vins, 220 ACB, 213 leaf, 197 veff, 39 vtex and 35
vmdl entries.

`chr\sch\pc\cmn\btlcmn` holds the character schedulers:

| Scheduler entry | Clips (start frame) |
| --- | --- |
| `chr\sch\pc\cmn\normal_dam\bin\normal_dam` | BindActorClip 0, RaptureActionSelectDamageMccClip 0 |
| `chr\sch\pc\cmn\dk_only\bin\dk_only` | BindActorClip 0, RaptureActionSelectDamageMccClip 0 |
| `chr\sch\pc\cmn\fadeout\bin\fadeout` | BindActorClip 0, RaptureCharaColorFadeClip 0 |

`InGame_Vfx` holds the damage schedulers. Each starts with BindActorClip
and one ActionClip at frame 0 whose action id `u32(D+0x10)` is 1. The
ActionClip end and any further clips are:

| Schedulers | ActionClip end | Further clips at frame 0 |
| --- | ---: | --- |
| `dkpv_s`, `dkpv_m`, `dkpv_l`, `dkpv_crit`, `dkdt_norm`, `dkdt_kill` | 380000 | none |
| `dkdt_knoc`, `dkdt_knor`, `dkdt_knol` | 380000 | RaptureEffectAtoBClip |
| `dkdt_mshr`, `dkdt_mshl` | 320000 | RaptureEffectAtoBClip |
| `dkdt_stsk`, `dkbl_aqua` | 320000 | RaptureSoundClip, RaptureEffectAtoBClip |
| `dkdt_sway` | 280000 | none |
| `dkdt_shdr`, `dkdt_shdl` | 280000 | RaptureEffectAtoBClip |
| `dkdt_blin` | 280000 | RaptureSoundClip, RaptureEffectAtoBClip |
| `dkdt_cove` | 260000 | none |
| `dkdt_pier` | 260000 | RaptureEffectAtoBClip |
| `dkbl_prot` | 220000 | RaptureSoundClip, RaptureEffectAtoBClip |
| `dkbl_shel`, `dkbl_refl` | 220000 | RaptureEffectAtoBClip |
| `dkpa_zan`, `dkpa_totu`, `dkpa_da` | 180000 | none |
| `dkpa_nage`, `dkpa_ya` | 140000 | none |
| `dkpa_sya` | 120000 | none |

The scheduler entry paths group them as `sch\vomume\dkpv_*`,
`sch\damtype\dkdt_*` and `sch\buturi\dkpa_*`. `01.DAT` (a plain RES with
the same five top entries) holds `normal_dam` in its `btlcmn` RES and no
scheduler whose name starts with `dk`.

## RaptureActionSelectDamageMccClip enter

STATIC. The RTTI catalog
([ffxivgame.rtti.json](../../config/ffxivgame.rtti.json)) gives
`RaptureActionSelectDamageMccClip` the 42-slot vtable at RVA `0xc1461c`
(VA `0x0101461C`). Slot 1 is `0x00821390`.

`0x00821390(scheduler)`:

- binds the actor of ordinal `u8(D+3)` through `0x00A1FFA0`
  (`0x008213A8`) and returns unless its virtual `+0x1C` is true
- casts it with its virtual `+0x5C` and `0x009DA6CC` from the type
  descriptor `0x012B7C48` (`.?AVIActor@ISceneObject@Common@Engine@CDev@SQEX@@`)
  to `0x012B7810` (`.?AVCharaActor@Chara@Actor@Scene@Application@@`)
  (`0x008213BC..0x008213E0`)
- reads the vfx parameter through `+0xF0` virtual `+0x6C` of the object
  `0x00A20070` returns for the scheduler, and requires the parameter
  nonnull with dword `+4` equal to `0x00766678` (`0x0082140C`)
- calls `0x0065E550(chara, parameter)` with ECX at chara `+0x11B4`
  (`0x00821419`)

`0x0065E550` returns when the chara's virtual `+0x268` is nonzero and
otherwise calls `0x007B71B0(parameter)` with ECX at chara `+0xFC0`.

`0x007B71B0` returns when the virtual `+0x268` of its owner is nonzero.
When the high byte of the record word `+0x78` is `0x11` or `0x19` it
appends the record to the container at `this+0x34` through `0x007C86E0`
(`0x007B726A`). Otherwise it copies the record into a `0x20C`-byte local
through `0x007C7F80`, stores the float -1.0 (`0x00F62F80`) at copy
`+0x208`, appends the copy to the container at `this+0x48` through
`0x007C8D10` and destroys the local through `0x007C2DA0`
(`0x007B7216..0x007B7263`). In both cases, when record `+0x74` differs from
the owner's `+0xDC` it calls `0x0065B2C0(+0x74)` (`0x007B7273..0x007B7281`).

`0x007C21B0` has one caller, `0x00667F04` in `0x006679C0`. While byte
`+0xB3C` of its owner is 0 it pops each entry of the first container
(count `+0x30`) through `0x007B6400` (`0x007C21F0..0x007C2260`). It then
walks the second container (count `+0x58`). An entry whose `+0x208` is
below 0.0 goes to `0x007BFBD0(entry, 0.0)` (float `0x00FA4460`). An entry
at or above 0.0 has the float argument subtracted from `+0x208`
(`0x007C230B..0x007C2327`) and, once that is at or below 0.0, goes to
`0x007BFBD0(entry, -0.0 - value)`. After either call `+0x208` holds
-10000.0 (`0x00FA7B24`, stored at `0x007C23F5`). The call is at
`0x007C23C6`. `0x007BFBD0` is the only caller of `0x007A1810`
(`0x007C0CA3`).

INFERRED. The clip enter only queues the record. The receiver starts its
damage schedulers from the frame update of `0x006679C0`, in the update
after the enter for a record of the second container. The removal of
entries marked -10000.0 and the route of the first container to
`0x007BFBD0` were not traced.

## Result decoder kinds 1 and 2

STATIC. `0x007A1810(record, index)` returns when the virtual `+0x268` of
the receiver `[this]` is nonzero. It reads
`E = u32(record + 0x68 + 0x14 * index + 0x40)`, which is record `+0xA8` for
row 0, and binds the source actor `0x007A0150(record+0x74)`. Each start
below is `0x00662570(name, source+0xDC, 0)` with ECX the receiver.

- Bits 15 to 18 of E, minus 1, index the four-entry jump table
  `0x007A26D8`. Entries 0 to 3 load the scheduler pointers `0x012C4E0C`,
  `0x012C4E10`, `0x012C4E14` and `0x012C4E18`. A nonnull one is started on
  the source actor when the dword `record+0x68` passes a time compare
  against `this+0x6C`, which it then replaces (`0x007A1870..0x007A18BA`).
  Not followed further.
- The kind `(E >> 27) & 0xF`, minus 1, indexes the 14-byte table
  `0x007A2700` into the six-entry jump table `0x007A26E8`
  (`0x007A18BF..0x007A18DC`). Kinds 1 and 2 take `0x007A19E1`, kind 6
  `0x007A18E3`, kinds 12, 13 and 14 `0x007A20EE`, `0x007A1EA4` and
  `0x007A25D6`, and kinds 0, 3 to 5, 7 to 11 and 15 return through
  `0x007A26CE`.

Arm `0x007A19E1` splits E into `(E >> 9) & 0x3F`, `(E >> 6) & 7`,
`(E >> 2) & 0xF` and `E & 3` (`0x007A19E1..0x007A19FF`) and starts at most
one name from each field, in that order.

`(E >> 9) & 0x3F` indexes the 28-entry jump table `0x007A2720`. A value
above 27 continues with the next field.

| Value | Action | String |
| ---: | --- | --- |
| 0, 27 | return | |
| 1 | start `dkdt_sway`, return | `0x00FE6884` |
| 2, 21, 22, 23, 26 | continue | |
| 3 to 10 | start `dkdt_norm`, `dkdt_kill`, `dkdt_cove`, `dkdt_pier`, `dkdt_shdr`, `dkdt_shdl`, `dkdt_mshr`, `dkdt_mshl`, continue | `0x00FE6890..0x00FE68E4` |
| 11 to 20 | start `dkdt_kb_1`, `dkdt_kb_2`, `dkdt_kb_3`, `dkdt_kb_4`, `dkdt_kb_5`, `dkdt_kbr1`, `dkdt_kbr2`, `dkdt_kbl1`, `dkdt_kbl2`, `dkdt_kb_5`, continue | `0x00FE68F0..0x00FE695C` |
| 24 | start `dkdt_blin`, return | `0x00FE6968` |
| 25 | start `dkdt_stsk`, continue | `0x00FE6974` |

`(E >> 6) & 7` starts `dkbl_prot` for 1 (`0x00FE6980`), `dkbl_shel` for 2
(`0x00FE698C`) and `dkbl_refl` for 3 (`0x00FE6998`), and nothing otherwise
(`0x007A1BA3..0x007A1BE3`).

`(E >> 2) & 0xF`, minus 1, indexes the five-entry jump table `0x007A2790`:
`dkpa_zan`, `dkpa_totu`, `dkpa_da`, `dkpa_sya` and `dkpa_mu`
(`0x00FE69A4..0x00FE69D0`). Other values start nothing.

`E & 3` indexes the jump table `0x007A27A4`: `dkpv_s`, `dkpv_m`, `dkpv_l`
and `dkpv_crit` (`0x00FE69D8..0x00FE69F0`).

The arm then reads the signed byte `record+0x0D` (`0x007A1CA7`). When it is
positive it calls `0x0065FC40(record+0x68, 0)` on the source actor, and
values 1 to 18 index the jump table `0x007A27B4`. Those arms were not
followed.

Worked values for this arm:

| E | Kind | Starts |
| --- | ---: | --- |
| `0x08000608` | 1 | `dkdt_norm`, `dkpa_totu`, `dkpv_s` |
| `0x0800060B` | 1 | `dkdt_norm`, `dkpa_totu`, `dkpv_crit` |
| `0x08000E04` | 1 | `dkdt_shdr`, `dkpa_zan`, `dkpv_s` |
| `0x08000200` | 1 | `dkdt_sway` |
| `0x08000000` | 1 | nothing |

MEASURED. `dkdt_kb_1` to `dkdt_kb_5`, `dkdt_kbr1`, `dkdt_kbr2`,
`dkdt_kbl1`, `dkdt_kbl2` and `dkpa_mu` name no scheduler of
`data/15/D9/00/00.DAT` or `01.DAT`. Every other name the arm starts names
one in `00.DAT`.

## Element name tables

STATIC. `0x00855160` has one caller, `0x00664A52`, with ECX at actor
`+0x143C`. It resolves 41 names from the pointer table `0x012D13C0` into
dwords `+0x00..+0xA0` (count `0x29` at `0x00855183`): `EID_NONE`, the eight
names `EID_L_FOOT` to `EID_R4_FOOT`, `EID_V01` to `EID_V30`,
`EID_SE_FOOT_R` and `EID_SE_FOOT_L`. It then resolves the 16 names of
`0x012D1380` into `+0xA4..+0xE0`: `EID_DAM_N`, `EID_DAM_NNE`, `EID_DAM_NE`,
`EID_DAM_NEE`, `EID_DAM_E`, `EID_DAM_SEE`, `EID_DAM_SE`, `EID_DAM_SSE`,
`EID_DAM_S`, `EID_DAM_SSW`, `EID_DAM_SW`, `EID_DAM_SWW`, `EID_DAM_W`,
`EID_DAM_NWW`, `EID_DAM_NW` and `EID_DAM_NNW`.

Each name goes to `0x0065BD70(name, 9)` on the object at `+0xE4`, with
`0x007CCF10(name)` when that returns a negative value. `EID_CURRENT`
(`0x01043344`) skips the first lookup. `EID_CRAFT_MAT` (`0x01043350`) takes
its second argument from `0x0065AE60` in place of 9.

## Reproduction

```text
ghidra x-ref to 0x00fa4948
ghidra x-ref to 0x0126f4b0
ghidra x-ref to 0x007c21b0
ghidra x-ref to 0x007bfbd0
ghidra x-ref to 0x007a1810
ghidra x-ref to 0x00855160
ghidra disasm 0x0055fb40 -n 30
ghidra disasm 0x00821390 -n 60
ghidra disasm 0x007b71b0 -n 80
ghidra disasm 0x007a1810 -n 400
```

Read the dwords of `0x00FA4948` (4), `0x0126F4B0` (2), `0x007A26D8` (4),
`0x007A26E8` (6), `0x007A2720` (28), `0x007A2790` (5), `0x007A27A4` (4),
`0x007A27B4` (18), `0x012D13C0` (41) and `0x012D1380` (16), and the 14
bytes at `0x007A2700`, from the file-backed sections.

## Open

- The removal of entries marked -10000.0, the first container's route to
  `0x007BFBD0`, and the meaning of record words `+0x78`, `+0x68` and
  `+0x0D`.
- The arms of `record+0x0D`, the scheduler pointers `0x012C4E0C` to
  `0x012C4E18`, and kinds 6, 13 and 14.
- The writer of `this+0x94` before `0x0055FB40`, and what loads
  `0x15D90000` once it is requested.
- Which element an effect binds to when the skeleton lacks the name, and
  where the `EID_V*` elements are placed.
