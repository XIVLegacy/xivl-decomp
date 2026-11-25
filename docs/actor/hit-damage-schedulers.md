# Hit damage schedulers

The receiver of a hit plays its damage effects through schedulers of the
resident effect bank. This page records the resident id list the client
requests, the damage schedulers that bank holds, the route from the
`normal_dam` scheduler's RaptureActionSelectDamageMccClip to the result
decoder `0x007A1810`, the scheduler names the kind 1 and 2 arm of that
decoder starts, the element name tables of `0x00855160`, and the element
query that answers the facing elements `EID_V01` to `EID_V05`. The
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

`0x007C21B0` has one caller, `0x00667F04` in `0x006679C0`. It pops every
entry of the first container (count `+0x30`) and passes each to
`0x007B6400` only while byte `+0xB3C` of its owner is 0
(`0x007C21F0..0x007C2260`). It then
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
after the enter for a record of the second container. The route of the
first container to `0x007BFBD0` was not traced.

## Result drain `0x007C21B0`

STATIC. Each container is a ring of record pointers with data `+4`,
capacity `+8`, head `+0xC` and count `+0x10` from its base. The first
container is based at `+0x20`, the second at `+0x48` and the auto attack
container of the [clip enter](#raptureactionselectdamagemccclip-enter) at
`+0x34` (data `+0x38`, capacity `+0x3C`, head `+0x40`, count `+0x44`).

- After the second container's walk, when the head entry's `+0x208` is
  below -9999.0 (float `0x00FE81B8`), `0x007C4310` compacts that
  container (`0x007C2443..0x007C245C`).
- From `0x007C2470` the routine walks the `+0x34` container. For each head
  record it computes the binary64 10.0 at `0x00F67848` divided by the
  record float `+0x10` (`0x007C24E3..0x007C24FB`), allocates a
  `0x20C`-byte entry through `0x0044D500(0x20C, 0, 0x1F)` (`0x007C2543`)
  when the ring slot is empty, copies the record into it through
  `0x007C7F80` (`0x007C2572`), stores the quotient at `+0x208`
  (`0x007C2588`) and counts it into the second container
  (`0x007C25A0`). It then calls `0x007BB990(record)` with ECX the drain
  owner (`0x007C25DA`) and pops the record through its virtual `+0`
  (`0x007C25F4`).
- From `0x007C262A` the routine walks a further container (count `+0x1C`),
  which is not traced here.

INFERRED. A record of category `0x11` or `0x19` starts its guard or brace
motion in the drain after the enter, and its damage schedulers through
`0x007BFBD0` once `10.0 / f` units of the drain's float argument have
passed, f being the record float `+0x10`. The writer and unit of that
float are not traced. Any other record starts its damage schedulers in the
update after the enter and never reaches `0x007BB990` from this routine.

### Auto attack record float

STATIC. The only instruction found that stores the immediate `0x00766678`
is `0x0080C860` in `0x0080C830`, which writes it to the object's dword `+4`.
A byte search of the executable finds 19 occurrences of the immediate.
The other 18 are compares of a dword `+4` against it, the clip enter's at
`0x0082140C` among them. `0x0080C830` ends by calling `0x0080C550` at
`0x0080C8BF`.

STATIC. `0x0080C550` stores the float -1.0 (`0x00F62F80`) at `+0x14`
(`0x0080C570`) and 1.0 (`0x00F54F70`) at `+0x10` (`0x0080C57F`), 0 at the
bytes `+0xB` and `+0xE` and `0xFF` at the bytes `+0xC` and `+0xD`
(`0x0080C584..0x0080C58D`).

STATIC. The drain's float argument is the float argument of the CharaActor
update `0x006679C0` (`0x00667EF7..0x00667F04`), which the same routine adds
to the float `+0x195C` and splits into a whole count
(`0x00667A05..0x00667A2B`).

INFERRED. A vfx parameter that keeps the constructor's `+0x10` gives an auto
attack record the countdown 10.0. A writer of `+0x10` after construction
was not found and is not excluded. The unit of the update argument was not
traced to its source, so the delay in seconds is not established here.

## Guard and brace motion `0x007BB990`

STATIC. `0x007BB990(record)` returns when the receiver's virtual `+0x268`
is nonzero and when `0x007A0150(record+0x74)` finds no source actor
(`0x007BB990..0x007BB9C9`). It then finds the receiver's row.

- With the signed byte `record+0xC` at or above 0 it walks the list of
  `0x0080B0F0` and `0x0080B020` for an entry whose `+8` is the receiver's
  `+0xDC` and whose row byte `record + 0xAF + 0x14 * i` equals
  `record+0xC` (`0x007BB9D3..0x007BBA25`).
- With `record+0xC` negative it takes the first of the u16 `record+0x98`
  rows whose target `record + 0xA0 + 0x14 * i` is the receiver's `+0xDC`
  (`0x007BBA33..0x007BBA6A`).

The row code is the low nibble of `record + 0xAE + 0x14 * i`
(`0x007BBA78..0x007BBA87`). The stance flag is set when the receiver byte
`+0xB39` is 2 or `0x51` (`0x007BBA8B..0x007BBAA0`). The routine returns
unless the kind `(E >> 27) & 0xF` of the row effect dword E is 1 or 2
(`0x007BBAA2..0x007BBAB1`). When the signed byte `record+0xB` is nonzero it
calls `0x0065DE40` with the row code, a side value from the jump table
`0x007BBD60` and E with bits 28 to 30 cleared and bit 27 set
(`0x007BBAB7..0x007BBB12`), not followed here.

It then takes `(E >> 9) & 0x3F` minus 3 into the 18-entry jump table
`0x007BBD80` (`0x007BBB17..0x007BBB28`). A value above 17 plays nothing.
Each arm passes one name to `0x007BA0E0`.

| Damage type | Motion name | Strings |
| ---: | --- | --- |
| 3, 4 | `cbbm_dmg_brace` with the stance flag, `cbnm_dmg_brace` without it, only when `0x007A3EE0(receiver+0x590)` is true and the row code is 1 | `0x00FE6248`, `0x00FE6258`, `0x00FE6268`, `0x00FE6278` |
| 5, 6 | none | |
| 7, 9 | `cbbm_grd1_f_1` with or without the stance flag | `0x00FE6288`, `0x00FE62A8` |
| 8, 10 | `cbbm_grd2_f_1` with or without the stance flag | `0x00FE6298`, `0x00FE62B8` |
| 11 to 20 | `cbbm_dmg_brace` with the stance flag, `cbnm_dmg_brace` without it | `0x00FE62C8..0x00FE63F8` |

The table `0x007BBD80` holds `0x007BBB2F`, `0x007BBB6D`, `0x007BBB63`
twice, `0x007BBBAB`, `0x007BBBC3`, `0x007BBBDB`, `0x007BBBF3`, then
`0x007BBC0B` to `0x007BBD3D` in steps of `0x22`.

STATIC. `0x007BA0E0(name)` returns false when the receiver's virtual
`+0x268` is nonzero, its byte `+0xB3C` is nonzero, its byte `+0xB3D` is 1
to 13, `0x007A5320` on receiver `+0xB80` answers false, or `0x007ADA50` on
receiver `+0x4C0` answers true (`0x007BA0E0..0x007BA138`). Otherwise it
calls `0x007ACD60` on `+0x4C0`, passes the name to `0x007B9920`
(`0x007BA162`) and, on success, to `0x007B1030` (`0x007BA17E`). It tries no
second name.

INFERRED. The stance flag is the battle stance, so an auto attack that the
receiver guards plays `cbbm_grd1_f_1` or `cbbm_grd2_f_1` in or out of
battle, and a brace plays the `cbbm_` name in battle and the `cbnm_` name
out of it.

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

STATIC. `0x007CCF10` jumps to `0x00A5F960`, which tail calls virtual `+0x10C`
of the object at its `this` `+0x0C` with the name. RaptureModelObject vtable
`0x010653A4` holds `0x008DE8D0` in that slot. It returns -1 when `+0xD8` is
null and otherwise jumps to the drawable name lookup `0x008DD880`, whose
provider, skeleton and `CDevDefault` order is on the
[nameplate attachment page](nameplate-layout-and-attachment.md#eid_name-model-record-and-world-position).
The matrix side `0x00A5FEF0` calls virtual `+0x11C` of the same member when
its virtual `+0x14` returns nonzero. In that vtable the slot is `0x008DEE90`,
which calls the drawable index getter `0x006693C0`. INFERRED: when the
fallback object is a RaptureModelObject, a name that the helper lookup misses
can resolve to a model joint. The run-time class at `+0x0C` was not proved.

## Element query and facing elements

STATIC. CharaActor slot 93 is `0x0065D4D0` (vtable cell `0x00FC0EA8`). It
calls `0x00855790(out, id, A, B, 0)` with ECX at actor `+0x143C`
(`0x0065D4D0..0x0065D4F5`). `0x00855790` dynamic-casts A and B to
CharaActor through `0x009DA6CC` (`0x008557A8`, `0x008557C2`). With the last
argument 0 it searches the 41 dwords at helper `+0x00` for `id`
(`0x008557E0..0x008557EB`). A match at index i makes the selector -i. A
nonzero last argument, or no match, keeps `id` as the selector.

A selector of 0 or above takes the ordinary path at `0x00855871`. A
negative selector -i with i from 1 to 40 indexes the byte table
`0x008558BC` at i - 1 and the jump table `0x008558A8`
(`0x00855806..0x00855812`). A selector below -40 takes the ordinary path:

| Index i | Names | Arm |
| --- | --- | --- |
| 1 to 8 | the eight foot names | `0x008552D0(out, A, id)` at `0x00855822` |
| 9, 11, 13 | `EID_V01`, `EID_V03`, `EID_V05` | `0x008553F0(out, A, B)` at `0x00855838` |
| 10, 12 | `EID_V02`, `EID_V04` | `0x008553F0(out, B, A)` at `0x0085584E` |
| 14 to 38 | `EID_V06` to `EID_V30` | the ordinary path |
| 39, 40 | `EID_SE_FOOT_R`, `EID_SE_FOOT_L` | `0x00855610(out, A, B)` at `0x00855864` |

The byte table holds 0 eight times, then 1, 2, 1, 2, 1, then 4 twenty-five
times, then 3 twice. The jump table holds `0x00855819`, `0x0085582F`,
`0x00855845`, `0x0085585B` and `0x00855871`.

The ordinary path calls `0x0065BD60(selector)` on the owner at helper
`+0xE4`. That routine adds `0x1960` and jumps to `0x00846DC0`, which tests
`selector & 0xFF8000`. When the test holds, `0x0065BDC0` answers.
Otherwise `0x007CCF70` calls `0x00A5FEF0(out, selector)` on the owner.
`0x00A5FEF0` asks the object at owner `+0x0C` its virtual `+0x14`. When
that is true it answers that object's virtual `+0x11C(out, selector)`, else
it copies the 64 bytes that the object's virtual `+0x64` returns
(`0x00A5FEFC..0x00A5FF4C`).

### Foot element arm

STATIC. `0x008552D0(out, A, id)` answers the foot names of indexes 1 to 8.
It calls A's virtual `+0x64(out, id)`, the node transform of the element
(slot 25, `0x008552E4..0x008552F8`), then A's virtual `+0x88(tmp)`, the
position (`0x008552FA..0x00855309`). It stores the float at position `+4`
(y) into `out+0x34` (`0x0085530B..0x00855312`) and returns `out`. The
answer is the posed element matrix with its translation y replaced by the
height of A's position. The other 15 floats come from the node transform
unchanged.

### Nearest damage element

STATIC. `0x008553F0(out, P, Q)` returns a translation matrix through
`0x0061F3B0`, which writes the rows `0x00FB7A60`, `0x00FB6D80` and
`0x00FB7A70` (the identity rows) and the vector it is given with w set to
1.0 (`0x00F54F70`).

- With P and Q both null the vector is `0x00FBF760`, (0, 0, 0, 1)
  (`0x0085541E`).
- With one of them null the vector is the other's virtual `+0x88`
  (`0x00855441..0x00855454`, `0x00855461..0x00855474`).
- With both, it reads Q's and P's virtual `+0x88`. When the xyz distance
  between them is below 0.001 (`0x00FB7AB4`) the vector is the translation
  at `+0x30` of Q's virtual `+0x17C(buffer, 0)` (`0x008554BB..0x0085551F`).
- Otherwise it walks Q's virtual `+0x17C(buffer, i)` for i from 0 to 15,
  takes each translation minus P's position, zeroes the y lane, and keeps
  the translation whose length is strictly below the best so far, starting
  from `FLT_MAX` at `0x00FBA384` (`0x0085552C..0x008555AE`). The rows and
  the kept translation are written at `0x008555B0..0x008555E6`.

So the answer has no rotation, and the facing of either actor does not
turn it. CharaActor slot 95 is `0x0065D530` (cell `0x00FC0EB0`), which
calls `0x00855330(out, i)` with ECX at actor `+0x143C`. `0x00855330` reads
the id at helper `+0xA4 + 4 * i`, one of the 16 `EID_DAM_*` ids that
`0x00855160` stores (`0x0085534F`). A negative id answers the translation
of the owner's virtual `+0x88`. Any other id takes the same
`0x0065BD60`, `0x0065BDC0` and `0x007CCF70` path as the ordinary path
above (`0x00855356..0x008553C8`). CharaActor slot 94 reaches the same
table entry 9 through `0x008558F0`, which calls `0x00855790` with id -9
and last argument 1 (`0x008558FD..0x00855904`).

### RaptureVfxActor binding

STATIC. RaptureVfxActor has two vtables. The COL `0x01164DFC` of the one
at `0x00FF54FC` (RVA `0xbf54fc`) gives object offset 0, and the COL
`0x01164E68` of the one at `0x00FF543C` (RVA `0xbf543c`) gives offset
`0x1C`.

- Slot 30 of the offset 0 vtable, `0x0080A660`, takes the bound actor at
  `+0x50` and looks up the dwords `+0x58` and `+0x5C` through `0x007A0150`
  on that actor's `+0x118` object plus `0x54`. With neither found it
  answers the bound actor's virtual `+0x64(out, id)`. Otherwise it answers
  the bound actor's virtual `+0x174(out, id, A, B)`, slot 93, with A the
  `+0x58` result and B the `+0x5C` result (`0x0080A665..0x0080A6C7`).
- Slot 21 of the offset `0x1C` vtable, `0x0080B270`, reads the same object
  fields at its `+0x34`, `+0x3C` and `+0x40` and makes the same two calls.
  When its `+0x44` (object `+0x60`) is nonzero it passes the answer to
  `0x0080AA50` with ECX at the object start (`0x0080B2D9..0x0080B30D`).
  `0x0080AA50` was not followed.

INFERRED. A and B are the source and the target of the effect, so
`EID_V01`, `EID_V03` and `EID_V05` answer the damage element of B nearest
A, and `EID_V02` and `EID_V04` the damage element of A nearest B. The
writer of object `+0x58` and `+0x5C` was not traced. The getter is a
virtual slot, so its callers may ask again each update, and that was not
traced either.

## Reproduction

```text
ghidra x-ref to 0x00fa4948
ghidra x-ref to 0x0126f4b0
ghidra x-ref to 0x007c21b0
ghidra x-ref to 0x007bfbd0
ghidra x-ref to 0x007a1810
ghidra x-ref to 0x00855160
ghidra disasm 0x00855790 -n 90
ghidra disasm 0x008553f0 -n 150
ghidra disasm 0x0065d4d0 -n 12
ghidra disasm 0x00855330 -n 50
ghidra disasm 0x008558f0 -n 10
ghidra disasm 0x00a5fef0 -n 30
ghidra disasm 0x0080a660 -n 40
ghidra disasm 0x0080b270 -n 60
ghidra disasm 0x0055fb40 -n 30
ghidra disasm 0x00821390 -n 60
ghidra disasm 0x007b71b0 -n 80
ghidra disasm 0x007a1810 -n 400
ghidra disasm 0x007c21b0 -n 330
ghidra x-ref to 0x007bb990
ghidra disasm 0x007bb990 -n 260
ghidra disasm 0x007ba0e0 -n 50
```

For the drain and the guard motion, also read the dwords of `0x007BBD60` (8)
and `0x007BBD80` (18), the strings `0x00FE6248..0x00FE63F8` in steps of 16,
the binary64 at `0x00F67848` and the floats at `0x00FE81B8`, `0x00FA4460`
and `0x00FA7B24`.

Read the dwords of `0x00FA4948` (4), `0x0126F4B0` (2), `0x007A26D8` (4),
`0x007A26E8` (6), `0x007A2720` (28), `0x007A2790` (5), `0x007A27A4` (4),
`0x007A27B4` (18), `0x012D13C0` (41), `0x012D1380` (16) and `0x008558A8`
(5), the 14 bytes at `0x007A2700` and the 40 bytes at `0x008558BC`, the
floats at `0x00FBF760`, `0x00FB7A60`, `0x00FB6D80`, `0x00FB7A70`,
`0x00FB7AB4` and `0x00FBA384`, the cells `0x00FC0EA8` and `0x00FC0EB0`, and the COL dwords
before `0x00FF543C` and `0x00FF54FC`, from the file-backed sections.

## Open

- The first container's route to `0x007BFBD0`, the container walked from
  `0x007C262A`, any writer of the record float `+0x10` after
  `0x0080C550`, the unit of the update argument, and the
  meaning of record words `+0x78`, `+0x68` and `+0x0D`.
- The row code byte, `record+0xB` and `0x0065DE40`.
- The arms of `record+0x0D`, the scheduler pointers `0x012C4E0C` to
  `0x012C4E18`, and kinds 6, 13 and 14.
- The writer of `this+0x94` before `0x0055FB40`, and what loads
  `0x15D90000` once it is requested.
- Which element an effect binds to when the skeleton lacks the name, the
  model answer (virtual `+0x11C`) for an element id it does not hold, and
  the arm `0x00855610`.
- The writer of RaptureVfxActor `+0x58` and `+0x5C`, and `0x0080AA50`.
