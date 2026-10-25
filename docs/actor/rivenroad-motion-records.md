# Rivenroad MotionClip names and resource entries

The selected m917 action banks contain 73 bounded SCB schedulers and 808
clip records. Thirty MotionClip records select local RIDTBL names. Twenty-two
records have same-bank MTB, MCB and CIBT name matches; five more have those
matches in the selected BID 0001 file. Three names have no match in this
nineteen-file selection. These are authored resource associations, conditional
on the [global MotionClip construction and callback context](../resource/combat-motion-name-provider.md).
They do not establish command dispatch, selected actor roots or playback.

Normal and hard encounter policy remains distinct and unassigned by these
resources. Bank numbers, scheduler labels and motion names do not select
a difficulty, command ID, phase threshold, spawn or authoritative AI.

## Input identity

All paths below are relative to `client/chara/mon/m917/act/2sp_emp/`,
with `base/` omitted. The [presentation contract](rivenroad-presentation.md#nael-and-fragment-resources)
owns the complete-file identities for BID 0000/0001 and WSS 0001, 0010,
0015 and 0018. The other thirteen input identities are below. Hashes cover
each complete original file. No file was selected from its apparent
encounter or difficulty meaning.

| Bank | Bytes | SHA-256 |
|---|---:|---|
| wss/0002 | 309184 | `172f9b706835b9f0444e65836e4569d2687080854ff3f17e3030ebda8500481e` |
| wss/0003 | 486624 | `7c40ed1d7e310d1174ce05c9432c16368b0491ce913cd19f814051ef30f077db` |
| wss/0004 | 379128 | `989b1111988df6e1f07ceb989f584c0b53cc17790de31a91cb95bcba92614d84` |
| wss/0005 | 255104 | `1e078bbb53a2ff65d82662106f578647a3cc6f335a707a48e823ea9c775f840d` |
| wss/0006 | 205920 | `730f1f5b602601d0daab20b4e47ca293ab618a7721c2c3b76b140f2139fae797` |
| wss/0007 | 628352 | `ea13465be07e23ea69bd4c724e52c669c9910cd6d1e0fd66828b2b2ae652e0d4` |
| wss/0009 | 460976 | `aa0e26d12af2d815920aa6aee67e0839f06ef45833a178d6ca3906880eb88823` |
| wss/0011 | 553712 | `2022edd3c23fb48912adb40afde457d17d8cea9682dda2bec746152603e27b3b` |
| wss/0012 | 276704 | `c43417e4b125c36803ff1dd376cd4073ac500ce5338b0e6c05037d4322d30c13` |
| wss/0014 | 285280 | `120ead6e460462fa68f89612a0453a43c2624efa22efe3c730c202c5eecbeb6e` |
| wss/0016 | 324912 | `67d838ad2481551fd3cec9b4b31c32a6803e459128cfcf722af85254e0de237c` |
| wss/0017 | 259424 | `afbe87aa466e27511b970477a174a31e276f4e1b4766ee710dd67e2752246b1c` |
| wss/0019 | 402208 | `2e2da1743e0944ead1c28f95b077c9a8eb1f9154ba4b9442e69fa081a5820eba` |

## BID MotionClip records

Every row below is in BID 0000. Offsets are absolute within that file.
Each record is 0x2c bytes, and every selected name is NUL-terminated
within its 16-byte RIDTBL entry. The provider therefore preserves it
without shortening. BID 0001 contains no SCB in this bounded RES walk.

| Scheduler label | Clip / name-entry offsets | RIDTBL index | Name | Matching selected bank |
|---|---|---:|---|---|
| dead | 0x71c2c / 0x71ce0 | 0 | cbbm_ded | bid/0001 |
| cas90 | 0x7223c / 0x722d0 | 0 | cbbm_wslp_1 | bid/0001 |
| cas00 | 0x726fc / 0x72790 | 0 | cbbm_abl_1 | bid/0001 |
| sht_can0 | 0x72bbc / 0x72c60 | 1 | cbbm_abl_3_can | None in selection |
| guard_off_1 | 0x7318c / 0x73240 | 1 | cbbm_grd1id_3 | bid/0001 |
| guard_on_1 | 0x737ac / 0x73860 | 1 | cbbm_grd1id_1 | bid/0001 |
| init_msb4_1 | 0x750d4 / 0x75190 | 1 | cbbm_msb4_1 | bid/0000 |
| init_msb5_1 | 0x75a64 / 0x75b20 | 1 | cbbm_msb5_1 | bid/0000 |
| init_msb6_1 | 0x76404 / 0x76510 | 0 | cbbm_msb6_1 | bid/0000 |
| init_msb7_0 | 0x9985c / 0x99928 | 1 | cbxs_st0 | None in selection |
| init_msb7_1 | 0x99dfc / 0x99eb8 | 1 | cbxs_st1 | None in selection |

The five BID 0001 joins identify companion resources in another selected
file. They do not prove that a particular actor loaded both BID files.
The unmatched names are cbbm_abl_3_can, cbxs_st0 and cbxs_st1. No absence
outside the selected files is claimed.

## WSS MotionClip records

Every listed WSS name has same-bank MTB, MCB and CIBT entries. Offsets
and byte counts in resource cells are `file offset / payload bytes`.
Multiple entries are retained rather than choosing one by position.

| WSS | Clip / name-entry offsets | RIDTBL index | Name | MTB | MCB | CIBT |
|---|---|---:|---|---|---|---|
| 0001 | 0x11404 / 0x11544 | 2 | cbbm_sp_01 | 0x3f8 / 66973 | 0xe0 / 792 | 0x10998 / 44 |
| 0002 | 0xe064 / 0xe194 | 2 | cbbm_sp_02 | 0x3e8 / 53773 | 0xd0 / 792 | 0xd5f8 / 44 |
| 0003 | 0x44728 / 0x44880 | 3 | cbbm_sp_03 | 0x3e8 / 41949; 0x39158 / 41949 | 0xd0 / 792; 0x38e40 / 792 | 0xa7c8 / 44 |
| 0004 | 0x86e8 / 0x8834 | 3 | cbbm_sp_04 | 0x3e8 / 30909; 0x29f78 / 30909 | 0xd0 / 792; 0x29c60 / 792 | 0x7ca8 / 44 |
| 0005 | 0x2c6ec / 0x2c8a4 | 2 | cbbm_sp_06 | 0xd0 / 19725; 0x26b88 / 19725 | 0x4de0 / 792; 0x26870 / 792 | 0x50f8 / 44 |
| 0006 | 0x23318 / 0x233c4 | 2 | cbbm_sp_07 | 0xd0 / 20237; 0x1d6e8 / 20237 | 0x4fe0 / 792; 0x1d3d0 / 792 | 0x52f8 / 44 |
| 0007 | 0xb95c / 0xba24 | 1 | cbbm_sp_08 | 0xe0 / 43661 | 0xab70 / 792 | 0xae88 / 44 |
| 0009 | 0xf1c0 / 0xf318 | 3 | cbbm_sp_b01 | 0x3f8 / 58141 | 0xe0 / 792 | 0xe718 / 64 |
| 0010 | 0x23344 / 0x23550 | 2 | cbbm_sp_10 | 0x3a8 / 14317; 0x1ee98 / 14317 | 0xd0 / 728; 0x1ebc0 / 728 | 0x3b98 / 44 |
| 0011 | 0x62420 / 0x62700 | 4 | cbbm_sp_11 | 0x3b8 / 55933; 0x4ab18 / 55933 | 0xe0 / 728; 0x4a840 / 728 | 0xde38 / 44 |
| 0012 | 0x2b190 / 0x2b2b0 | 4 | cbbm_sp_12 | 0x3a8 / 18989; 0x25418 / 18989 | 0xd0 / 728; 0x25140 / 728 | 0x4dd8 / 44 |
| 0014 | 0xbac4 / 0xbc04 | 2 | cbbm_sp_13 | 0x3b8 / 44189 | 0xe0 / 728 | 0xb058 / 44 |
| 0015 | 0xd638 / 0xd7d4 | 4 | cbbm_sp_14 | 0xe0 / 51197 | 0xc8e0 / 728 | 0xcbb8 / 44 |
| 0016 | 0xd628 / 0xd774 | 2 | cbbm_sp_15 | 0x100 / 51133 | 0xc8c0 / 728 | 0xcb98 / 44 |
| 0017 | 0x2aeb0 / 0x2afe8 | 4 | cbbm_sp_09 | 0xd0 / 29277; 0x22be8 / 29277 | 0x7330 / 792; 0x228d0 / 792 | 0x7648 / 44 |
| 0018 | 0xbc0 / 0xdb4 | 5 | cbbm_ws02_1 | 0x41528 / 92557 | 0x41250 / 728 | 0xf0 / 44 |
| 0018 | 0xcb8 / 0xde4 | 8 | cbbm_ws02_2 | 0x58190 / 36941 | 0x57eb8 / 728 | 0x11c / 24 |
| 0019 | 0x3eb44 / 0x3ece8 | 2 | cbbm_ws_t_1 | 0x150f8 / 41485 | 0x14e20 / 728 | 0x660 / 44 |
| 0019 | 0x3ec18 / 0x3ed18 | 5 | cbbm_ws_t_2 | 0x1f5e0 / 29133 | 0x1f308 / 728 | 0x68c / 24 |

WSS 0003, 0004, 0005, 0006, 0010, 0011, 0012 and 0017 each contain
two MCB and two MTB entries matching the selected name. These are
distinct directory entries, without an asserted choice at runtime.
The [resource provider profile](../resource/actor-motion-resource-provider.md)
explains why retained root order and traversal precedence matter.

The [Rivenroad presentation contract](rivenroad-presentation.md) owns
the separate command/effect observations. In particular, these local
name bindings do not establish the WSS 0018/0019 command associations.

## Parsing and reproduction

Python 3.12.0 standard-library readers authenticated every input above,
walked only bounded RES directories and applied the native RIDTBL name
table formula. `tools/scb_clip_scheduler.py` supplied actor/class and clip
record boundaries. Original RESOURCE_TYPE entries supplied native types
`0x006D7462` (MTB), `0x006D6362` (MCB) and `0x63696274` (CIBT).
Name matches compare complete final components of original RES names.

Across all 808 clip records, the separate signed word `s16(D+0x0c)` equalled
the record's zero-based position in its SCB's all-block order. This is the
[built-list clip index](../resource/cut-scheduler-clip-timing.md#clip-index-word),
distinct from the RIDTBL name index at D+0x10 in the tables above. It does not
establish a selected block or invocation.

All nonmetadata child extents and the needed metadata prefixes fit their
owning views. No metadata declaration overran its view in this selection.
The nine separate Castrum metadata overruns retain their
[documented boundary](castrum-presentation.md#parsing-boundary).
Opaque effect/model leaves, MCB/CIBT payload behavior and MTB poses were
not decoded here. The separate m917/e001 s_awl_on EffectEnd target
mismatch remains as recorded in the presentation contract.

These static checks do not establish a live actor, current roots, native
lookup result, lower motion-object conversion, callback context or
successful playback. They do not reconstruct normal/hard server policy.
