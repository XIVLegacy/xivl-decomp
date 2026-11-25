# Attack scheduler selection

A battle result that reaches the attack presenter packs its rows into
12-byte entries and asks the bank of the receiver for a scheduler named
`<prefix>1<a><e>_<b+1>_<d>`. This page records the entry packing, the
name builder and its retry order, the per-actor variant cycle, the pass
table, the player dispatch and the reaction motion names. It answers part of
the open question of the
[battle bank finding](weapon-set-directories-and-battle-banks.md#open) about
which scheduler of a bank plays.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
base `0x00400000`. All native addresses are VA.

Producing tools are `ghidra` CLI driving Ghidra `12.1.4` disassembly,
decompilation and cross-reference queries, and Python reads of the
file-backed PE sections for strings and table bytes. Decompiler output was
used to orient. Every lane, offset and constant below was read from
instruction text. Install measurements walk the RES directories of
`xivl-tools:docs/formats/sedb-res.md` and parse each `SEDBSCB` child with
`tools/scb_clip_scheduler.py`. Everything native here is static.

The staged record R of `0x007B6400` holds the
[combat result record](../net/combat-result-element.md#staging) at R+`0x68`.
R+`0x78` is that record's effect or animation ID (the selector), R+`0x98` its
row count and R+`0xA0` its first row. A row is 0x14 bytes with the target at
`+0x00`, the effect ID E at `+0x08` and the row ordinal byte at `+0x0F`.

## Row packing `0x007B6400`

`0x007B6400(this, R)` returns at once when the virtual `+0x268` of
`[this]` is nonzero. It sets a category flag when
`(u32(R+0x78) & 0xFF000000) == 0x19000000` (`0x007B643E..0x007B6454`). For
each row with nonzero E whose kind `(E >> 27) & 0xF` is 1 or 2
(`0x007B64A3..0x007B64AD`), and whose target resolves through `0x007A0150`,
it fills one 12-byte entry. Rows of other kinds are skipped. Entry dword 2 is
the row target.

Entry dword 0:

| Bits | Source | Locator |
| --- | --- | --- |
| 0 | 1 when `E & 0x7E00` is `0xA00`, lane `(E >> 9) & 0x3F` equal to 5 | `0x007B6502..0x007B6508` |
| 1 to 4 | low four bits of the row ordinal byte `+0x0F` | `0x007B677F..0x007B6789` |
| 5 to 9 | a direction byte from a 16-entry stack table, indexed by a rounded angle | `0x007B6755..0x007B6777` |
| 10 to 13 | `(E >> 15) & 0xF` | `0x007B6791..0x007B6798` |
| 14 to 17 | 4 with the category flag, else `E & 3` | `0x007B67A9..0x007B67D2` |
| 18 to 21 | the kind, 1 or 2 | `0x007B67E4..0x007B67FE` |
| 22 to 27 | the lane of the first later row whose kind is 1, 2 or 3, when that kind is 3, else 0. Rows of other kinds are passed over | decompiled loop, stores at `0x007B67E4..0x007B67FE` |
| 28 to 29 | 1 for lanes 7, 8 and `0x19` (`0x007B6542`, `0x007B6557..0x007B656C`), 2 when `0x007B0EA0(R+0x68)` is true (`0x007B6577`), else 0 | |

The direction table on the stack is 1, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3,
2, 2, 1. Its index comes from the positions of the receiver's actor and the
target, both through virtual `+0x88`, and a facing angle, through
`0x009DAEBA` and the binary64 constants at `0x00FC3320` and `0x00F62FE0`.
INFERRED: the five bits are a direction 1 to 8 of the target about the actor.

Entry dword 1 holds `(row index - 1) & 0xF` in bits 0 to 3 and bits 2 to 5
of E in bits 8 to 11 (`0x007B67F1..0x007B681E`). Its bits 4 to 7 come from a
second scan of the later rows that was not decoded here.

## Player dispatch

After the walk (`0x007B684C..0x007B68A6`):

- When `((u32(R+0x78) >> 12) & 0xFFF) - 1` is above 2, or when `0x0065DA60`
  returns true, it calls `0x007B4530` with type `0xC` (`0x007B6978..0x007B6987`).
- Otherwise it reads a type word at `this+4` when bits 18 to 21 of the first
  entry are 1, else at `this+8`. Types above `0xB` return. The index bytes at
  `0x007B69B8` and the cells at `0x007B69A4` send type 1 to `0x007A1720` and
  `0x007B27A0`, type 2 to `0x007B23B0`, type 8 to `0x007B2CA0`, type 9 to
  `0x007B3090`, and types 0, 3 to 7, 10 and 11 to `0x007B4530` with the type.

Only `0x007B23B0` is read further on this page. `0x007B6400` has one direct
caller, `0x007C222C` in `0x007C21B0`, and `0x007B23B0` one, `0x007B68CD`.

## Pass table

`0x007B23B0(key, R, entries, count)` sets a flag for category `0x19` from
R+`0x78` (`0x007B23E3..0x007B23F5`) and reads the pass count as the byte at
`0x00FE7FD0 + 0x14 * j`, where j is bits 14 to 17 of entry 0
(`0x007B23FA..0x007B2403`). A further flag marks the last pass
(`0x007B241D..0x007B2422`).

For each pass p and entry, `b` is the byte at `0x00FE7FD1 + 0x14 * j + p`.
A byte `0xFF` becomes 0 or 2 from `rand()` (`0x007B259B..0x007B25BE`). The
records hold:

| j | Passes | b by pass |
| ---: | ---: | --- |
| 0 | 2 | 2, 0 |
| 1 | 3 | 2, 0, 2 |
| 2 | 3 | 2, 0, 2 |
| 3 | 3 | 0, 2, 0 |
| 4 | 1 | 0 |

The same records carry a byte at `+4 + p` and an f32 at `+8 + 4p`, read after
the name builder returns (`0x007B261C..0x007B262B`, with the f32 saved at
`0x007B2638`). The `0x14`-byte records of j 0 to 4 hold:

| j | Byte `+4`, `+5`, `+6` | f32 `+8`, `+12`, `+16` |
| ---: | --- | --- |
| 0 | 3, 0, 0 | 1.5, 1.0, 0.0 |
| 1 | 0, 4, 0 | 1.5, 1.0, 1.0 |
| 2 | 0, 4, 0 | 1.5, 1.0, 1.0 |
| 3 | 0, 0, 5 | 1.5, 1.0, 1.0 |
| 4 | 0, 0, 0 | 1.0, 0.0, 0.0 |

For a nonzero scheduler, `0x007B23B0` takes an object from `0x007B22D0` at
`0x007B2697`. That routine requests `0x218` bytes for it (`0x007B232B`). The
stores are:

| Object field | Value | Locator |
| --- | --- | --- |
| `+0x208` | the scheduler | `0x007B26A9` |
| `+0x09` bits 3 to 6 | the byte at record `+4 + p` | `0x007B26E7..0x007B2705`, mask `0x78` |
| `+0x10` | the f32 at record `+8 + 4p` | `0x007B26FD` |
| `+0x08` bit 0 | 1 when p is the pass count minus 1 (`0x007B241D..0x007B2422`) | `0x007B26DA..0x007B26FA` |
| `+0x08` bits 4 and 5 | bit 4 set and bit 5 cleared | `0x007B26DA..0x007B26FA` |

An entry with bit 0 of dword 0 set first takes a path that uses the string
`atk_cover` (`0x00FE61FC`). When that path yields a scheduler it joins the
same stores at `0x007B2682`. It has cleared the byte (`0x007B2476`) and saved
1.0 from `0x00F54F70` as the f32 (`0x007B246E..0x007B247B`). No reader of
object `+0x09` or `+0x10` was traced, and neither was the order of the passes
in time.

The call at `0x007B2606` passes, nearest first:

| Argument | Source |
| --- | --- |
| a | bits 5 to 9 of entry dword 0 |
| b | the pass byte above |
| c | bits 28 to 29 |
| d | bits 18 to 21 |
| e | `rand() & 1`, or with the category flag `[this+0x68]` after `0x007A9800(key, last pass flag)` (`0x007B25C1..0x007B25E4`) |

## Name builder `0x007A95D0`

`0x007A95D0(a, b, c, d, e)` formats with `%s%d%d_%d_%d` (`0x00FE6238`) into a
16-byte buffer through `0x009D4F83` (`0x007A9641..0x007A968D`). The string is
`atk` (`0x00FE622C`), `rac` (`0x00FE6230`) or `lst` (`0x00FE6234`) for c of
0, 1 or 2, and `atk` for c of 3 or more. The four numbers are a, e, b + 1 and
d in that order.

It looks the name up as resource type word `0x00736362` (`scb`) through
virtual `+0x10` of `[this]` (`0x007A96FD..0x007A970F`) and starts it through
virtual `+0x214`, `0x00634860` and `0x00635690`. When the lookup or the start
returns null it retries (`0x007A977E..0x007A97CA`):

1. while e is above 0, e decreases by 1
2. else e is restored, and while b is above 0, b decreases by 1
3. else b is restored, and when a is above 1, a becomes 1
4. else a is restored, and when c is above 0, c becomes 0
5. else it returns null

## Variant probe and cycle

`0x007A5800(key)` stores the key at `+0x5C`, zero at `+0x60` and `+0x64`
and `0xFFFFFFFF` at `+0x68` (`0x007A5822..0x007A582B`). It then formats
`atk1<i>_1_1` for i from 0 to 7 (the pushes at `0x007A583A..0x007A5846`), sets
bit i of `+0x60` for each name that resolves and stores the highest such i
plus 1 at `+0x64` (`0x007A58FE..0x007A5906`).

`0x007A9800(key, flag)` probes again when `+0x5C` differs from the key or
`+0x60` is zero (`0x007A9809..0x007A9816`). With n = `[+0x64]`, the start is
`(last + 1) mod n`, or `(rand() >> 4) mod n` when `[+0x68]` is `0xFFFFFFFF`
(`0x007A981B..0x007A983B`). It takes the first set bit of `+0x60` at or above
the start, else the first set bit below it. It stores that bit index at
`+0x68` and clears the bit (`0x007A98BF..0x007A98D0`). With no set bit it
stores 0 (`0x007A9875`). With key 1, a nonzero flag, a chosen index 0 and n
above 1 it probes again and stores `(rand() >> 4) mod (n - 1) + 1`
(`0x007A9882..0x007A98B5`).

INFERRED. An auto attack, selector `0x19001000`, therefore asks for
`atk1<a><v>_1_<kind>` with v from the cycle over the probed `atk1<i>_1_1`
names, and the retry order falls back toward `atk10_1_1`.

## Reaction motion names

The `.rdata` range from `0x00FE6248` to `0x00FE67B8` holds 96 NUL-terminated
motion names in 12- and 16-byte slots. The first, `cbbm_dmg_brace`, has two
data references, `0x007BBB4F` and `0x007BBB5B`. The names are
`dmg_brace`, `dmgm_f`, `dmgm_r`, `dmgm_l`, `dmgm_b` and `dodge_f` in
`cbbm_`/`cbnm_` pairs, `grd1_f_1`, `grd2_f_1`, `grd1_f_2`, `grd1id_3`,
`grd2_f_2` and `grd2id_3` with the `cbbm_` prefix only, and `add_dmg_f`,
`add_dmg_l`, `add_dmg_r`, `add_dmg_b`, `add_dmgh_f`, `add_dmgh_l`,
`add_dmgh_r` and `add_dmgh_b` in `cbba_`/`cbna_` pairs. The function that
indexes the range and the row fields that choose an entry were not traced.

## Install agreement

MEASURED on the install of build `2012.09.19.0001`. Names are the last path
component of each RES directory entry. Ticks are scheduler ticks, 10000 per
frame under the [clip timing](../resource/cut-scheduler-clip-timing.md) page.

`client/chara/pc/c001/act/clw_clw/atk/base/0001` nests one RES,
`sch\pc\cmn\wep\clw_clw\s_atk01`, with six schedulers:

| Scheduler | MotionClip span | RaptureCasterSchClip start | Motion name string |
| --- | --- | ---: | --- |
| `main` | none | none | none |
| `atk10_1_1` | 0 to 280000 | 0 | `cbbm_atk1_r` |
| `atk10_1_2` | 0 to 290000 | 0 | `cbbm_atk1_l` |
| `atk11_1_1` | 0 to 360000 | 50000 | `cbbm_atk2_r` |
| `atk11_1_2` | 0 to 400000 | 50000 | `cbbm_atk2_l` |
| `atk12_1_1` | 0 to 380000 | 60000 | `cbbm_atk3_r` |

`main` holds BindActorClip, RaptureActionSelectAttackClip and two
RaptureCasterManagedSchClip records. Every caster clip payload of the five
others begins with `normal_dam`.

| Bank | Schedulers |
| --- | --- |
| `c001/act/swd_emp/atk/base/0001` | `main`, `atk10_1_1`, `atk11_1_1`, `atk12_1_1`, one motion each, `cbbm_atk1` to `cbbm_atk3` |
| `c001/act/2bw_emp/atk/base/0001` | `main`, `atk10_1_1` (`cbbm_atk1`) |
| `c001/act/clw_clw/btl/base/0001` | `main`, `atk10_` and `rac10_` with `1_2`, `2_1`, `2_2`, `3_1` and `3_2`. Each plays two or three MotionClip records in sequence |
| `mon/m011/act/emp_emp/btl/base/0001` | `main`, `atk10_1_1`, `rac10_1_1`, `atk10_3_1`, `rac10_3_1`, two MotionClip records each |
| `c001/act/clw_clw/wsc/base/0001` | none. The bank holds the motions `cbbm_ws_z_1`, `cbbm_ws_z_2`, `cbbr_ws_z_1` and `cbbr_ws_z_2` only |

The motion entries of the body banks `c001/act/<set>/bid/base/0001`:

| Set | Reaction entries |
| --- | --- |
| `emp_emp`, `2bw_emp` | `cbbm_dmg_brace`, `cbbm_dmgm_{b,f,l,r}`, `cbbm_dodge_f`, `cbba_add_dmg_f`, `cbbm_ded`, `cbbm_pb02_dmg` |
| `clw_clw` | the same plus `cbbm_grd1_f_1`, `cbbm_grd1_f_2`, `cbbm_grd1id_1`, `cbbm_grd1id_2lp`, `cbbm_grd1id_3` |
| `swd_emp` | the `clw_clw` set plus the matching `cbbm_grd2_*` entries |

No `cbnm_` reaction entry is present in those four banks. `gun_emp` has no
`bid/base/0001`.

`client/chara/mon/m011/act/emp_emp/bid/base/0000` holds 21 motions:
`cbba_add_dmg_f`, `cbbm_01f_lp0`, `cbbm_02f_lp0`, `cbbm_abl_2lp`,
`cbbm_activ`, `cbbm_deact`, `cbbm_ded`, `cbbm_dedpose`, `cbbm_dmg_brace`,
`cbbm_dmgm_b`, `cbbm_dmgm_f`, `cbbm_dmgm_l`, `cbbm_dmgm_r`, `cbbm_id0`,
`cbbm_pb04_dmg`, `cbbm_sp_a_2lp`, `cbbm_sp_b_2lp`, `cbbm_wekid_2lp`,
`cbnm_dedpose`, `cbnm_id0` and `cbnm_wekid_2lp`. It has no `03f` entry and no
walk start or stop entry.

## Reproduction

```text
ghidra disasm 0x007B6400 -n 420
ghidra decompile 0x007B6400
ghidra disasm 0x007B23B0 -n 320
ghidra disasm 0x007A95D0 -n 200
ghidra disasm 0x007A5800 -n 80
ghidra disasm 0x007A9800 -n 80
ghidra x-ref to 0x007B6400
ghidra x-ref to 0x007B23B0
ghidra x-ref to 0x00FE6248
```

Read the strings at `0x00FE6238`, `0x00FE622C`, `0x00FE6230`, `0x00FE6234`
and the range `0x00FE6248..0x00FE67C8`, the bytes at `0x00FE7FD0` in steps
of `0x14` for j 0 to 4, and the dispatch bytes at `0x007B69B8` and cells at
`0x007B69A4`.
For the install rows, read each RES directory of the named file, follow
nested RES entries, and parse each `SEDBSCB` entry with
`tools/scb_clip_scheduler.py`.

## Open

- The scheduler of a weaponskill, which the `wsc` bank does not hold.
- What a RaptureCasterSchClip with a `normal_dam` payload starts on the
  target.
- The indexer of the reaction name range and its inputs.
- The bodies of `0x007B27A0`, `0x007B2CA0`, `0x007B3090` and `0x007B4530`.
