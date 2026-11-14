# Weapon set directories and battle bank paths

The CharaVisual builds three directory names from the class tags of weapon
slots 0 and 1. The action queue hands those names to the CharaActionVisual,
whose character bank loader places a battle, craft or gathering category
bank under one of them. This page records the writer of the three names, the
hand-off, the posture classes of the loader and the path it builds.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. All native addresses are VA.

Producing tools are `ghidra` CLI `0.2.2` driving Ghidra `12.1.4` decompile,
disassembly, memory-read and cross-reference queries, and Python `3`
standard-library directory listings of a contributor-supplied install of
build `2012.09.19.0001`. Field offsets, literals and branch targets were read
from instruction text and image bytes. Install facts are directory names
only. Everything native here is static.

`H` is the CharaVisual at `actor + 0x2B5C`. Its three directory strings, the
path builder `0x006B8B70` and the `emp_emp` retry of load record `0x23` are
described in the [idle bank finding](idle-bank-selection.md). The four tag
arrays of `H` are described in the
[weapon slot finding](weapon-slot-attach-and-sheathe.md#class-tag-arrays).
`V` is the CharaActionVisual of the
[bank routing finding](animation-bank-routing.md), which also owns the packed
selector, the category table `0x00FE32D8` and the kind 7 posture switch.

## Writer of the three directory names

`0x006B8680` stores stage word 2. Before that it calls `0x006B5770(0, 9)`
(`0x006B86CC`), which refreshes the four tag arrays for all nine slots, and
builds three strings with the separator `_` (literals `0x00FD2B5C`,
`0x00FD2B60`, `0x00FD2B68`):

| Destination | Stored at | First part | Second part |
|---|---|---|---|
| `H+0x1D0`, the `bid` name | `0x006B87CD..0x006B87D3` | tag at `H+0x2F0` | tag at `H+0x2F4` |
| `H+0x224`, the `cid` name | `0x006B8926..0x006B892C` | tag at `H+0x314` | tag at `H+0x318`, or the first part again when that tag equals `emp` |
| `H+0x278`, the `pid` name | `0x006B8A7E..0x006B8A84` | tag at `H+0x338` | tag at `H+0x33C`, or the first part again when that tag equals `emp` |

`H+0x2F0` and `H+0x2F4` are slots 0 and 1 of the second tag array, the one
filled from byte 1 of the class index row. The `bid` name is therefore
`<byte 1 tag of slot 0>_<byte 1 tag of slot 1>`, and an empty slot 1 keeps
the literal `emp`. The `cid` and `pid` names use bytes 2 and 3 of the row and
repeat the slot 0 tag for an empty slot 1.

`xivl-client-data:docs/weapon-class-index-and-attach-table.md` lists the
installed row tags. The install agrees with the rule: the set directories
under `client/chara/pc/c001/act` that hold a `bid` directory are `2ax_emp`,
`2bw_emp`, `2sp_emp`, `2st_emp`, `clb_sld`, `clw_clw`, `emp_emp`, `gun_emp`,
`stf_emp`, `stf_sld`, `swd_emp` and `swd_sld`. The ones that hold a `cid`
directory are `alc_alc`, `arm_arm`, `blk_blk`, `cok_cok`, `gld_gld`,
`lth_lth`, `sew_sew` and `wod_wod`, and the ones that hold a `pid` directory
are `ani_ani`, `fel_fel`, `fsh_fsh` and `min_min`.

## Hand-off to the CharaActionVisual

`0x00845E80` creates a CharaActionQue. It fills a parameter block from the
actor `A` held at its receiver's `+0x228` and `H = [A + 0x2B5C]`: `H+0x17C`,
`H+0x1D0`, `H+0x224`, `H+0x278`, `H+0x35C` and the byte `A+0xB39`. The
constructor `0x00843B50` builds the CharaActionVisual (`0x00798AA0`) and
copies the block into it:

| Block dword | Source | Setter | Destination |
|---:|---|---|---|
| 2 | `H+0x17C` | `0x00843730` | string at `V+0x1D0` |
| 3 | `H+0x1D0`, the `bid` name | `0x00843790` | string at `V+0x224` |
| 4 | `H+0x224`, the `cid` name | `0x008437F0` | string at `V+0x278` |
| 5 | `H+0x278`, the `pid` name | `0x00843850` | string at `V+0x2CC` |
| 6 | `H+0x35C`, the five bank names | inline copy of 25 bytes | `V+0x320` |

Each setter adds its destination offset to the visual pointer
(`0x0084375D`, `0x008437BD`, `0x0084381D`, `0x0084387D`). The strings of `V`
are a snapshot taken when the queue entry is created.

## Posture classes

The loader passes the retained posture `V+0x28` to `0x006B71D0`
(`0x00798C2D..0x00798C3A`):

| Posture | Class |
|---|---:|
| `0xFFFFFFFF`, 0, 1, `0x46` | 0 |
| `0x1E`, `0x1F`, `0x20` | 2 |
| `0x32`, `0x33` | 3 |
| `0x0B` | 4 |
| `0x0D` | 5 |
| `0x0E` | 6 |
| `0x0F` | 7 |
| `0x5B` | 8 |
| `0x5C` | 9 |
| every other value | 1 |

## Character bank loader `0x00798BF0`

The loader takes the category, the character bank number and the route kind.
It loops a suffix index `k` from 0 to 4 and stops when the category or the
bank number is below 1. Each pass builds two path streams. Stream `A` starts
as the string at `V+0x1D0` followed by `act/` (`0x00FE3B98`).

- Route kinds 1 to 7 (`0x00798D4B`): `A` gets the string at `V+0x224` when
  the class is 1 (`0x00799003`), else the literal `cmn` (`0x00FE3BA0`), then
  `/`. Kind 7 appends `em1`, `em2` or `em3` by the tracked posture switch.
  The other kinds append the category name from virtual `+0x5C`.
- Route kind 12 (`0x00798D54`): `A` gets `V+0x224` when the class is 1
  (`0x00798DAB`), else `cmn` (`0x00FE3BA8`), then `/` and the category name.
- Every other route kind: stream `B` is a copy of `A` followed by `cmn`
  (`0x00FE3BB0`). `A` gets, through the four-cell jump table at
  `0x00799724`, `V+0x224` for classes 0 and 1 (`0x00798EDF`), `V+0x278` for
  class 2 (`0x00798EF0`), `V+0x2CC` for class 3 (`0x00798F01`) and nothing
  for classes 4 to 9. Both streams then get `/` and the category name.

Both streams end with `/`, the bank name (`base`, `0x00FE3BD8`, for `k` 0 and
the five-byte name at `V + 0x320 + 5 * k` otherwise), `/` and `%04d`
(`0x00FE3BE8`) of the bank number.

The resolve step tries `B` with `.bin` (`0x00FE3BF0`) first for route kind
16, and for route kind 9 when the class is not 1. In every other case, and
when that attempt is rejected, it tries `A` with `.bin` (`0x00FE3BF8`), but
only while no earlier suffix has loaded. An accepted path is loaded into
record `k + 4` through `0x006320C0`.

The category table `0x00FE32D8` gives route kind 0 to `cft` (16), `btl` (17),
`wsc` (18), `wss` (19) and `pic` (20), and route kind 17 to `atk` (25). For
an actor whose posture class is 0 or 1 those categories therefore resolve
`<V+0x1D0>act/<bid name>/<category>/<bank name>/<NNNN>.bin`. Stream `B` is
not tried for these kinds, so the loader has no `cmn` fallback and no
`emp_emp` retry for them. A kind 1 to 7 bank of an actor in posture class 1
sits under the `bid` name as well.

Install agreement, `client/chara/pc/c001/act`: `swd_emp`, `swd_sld`,
`2sp_emp`, `2ax_emp`, `2bw_emp`, `2st_emp`, `clw_clw`, `stf_emp` and
`stf_sld` each hold `atk`, `btl`, `wsc`, `em1` and `mgc` beside `bid`.
`emp_emp` holds `btl`, `em1` and `mgc` and no `atk`. `clb_sld` holds `btl`
only and `gun_emp` holds no category directory. The craft sets hold `cft`
and the gathering sets except `ani_ani` hold `pic`.

## Reproduction

```text
ghidra disasm 0x006B8680 -n 240
ghidra decompile 0x00845E80
ghidra decompile 0x00843B50
ghidra disasm 0x00843790 -n 30
ghidra decompile 0x006B71D0
ghidra decompile 0x00798BF0
ghidra disasm 0x00798BF0 -n 400
ghidra memory read 0x00799724 16
ghidra memory read 0x00fe3b98 104
ghidra memory read 0x00fe32d8 408
```

Install listing: enumerate the directories under
`client/chara/pc/c001/act` and their children.

## Open

- Which scheduler or motion of an `atk`, `btl` or `wsc` bank plays. A bank
  holds several of each.
- A set name the row tags can form and the install lacks for a model, such
  as `rap_emp` or `2sw_emp` under `c001`. The `bid` builder has the tracked
  `emp_emp` retry. The category loader above has none.
- The string at `H+0x17C` beyond its use as the path prefix.
