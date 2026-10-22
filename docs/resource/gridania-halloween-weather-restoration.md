# Gridania's original Halloween weather resource

The October 2011 Gridania weather-8027 layout was recovered from its complete
retail patch. Its 29 nonzero outer resource references all resolved in the
supplied final 1.23b install. The two references absent from Gridania's clear
weather table also matched their October patch bytes exactly. This supports a
one-file restoration candidate at `data/29/B0/00/20.DAT`, while retaining the
final region table and resident Halloween selectors. Rendering remains a
client acceptance question.

## Original patch and extraction

The source was `D2011.10.04.0000.patch`, archive member
`ffxiv_patches/48eca647/patch/D2011.10.04.0000.patch` in
[`ffxiv_patches.zip`](https://archive.org/details/ffxiv_patches).
The recovered patch had 677,633,296 bytes, CRC32 `95C15318` and SHA-256
`b052bb8b5642bc4ea12d42d7bb1f8ff4b1f595f23539039599067c56191393d8`.
These identities pin the examined input, independently of the mirror's label.

Python 3.12 `struct`, `zlib` and `hashlib` produced the extraction. The patch
began with `91 5A 49 50 41 54 43 48 0D 0A 1A 0A`. From offset `0x0C`, a
big-endian u32 body length preceded each four-byte chunk tag. The following
CRC32 covered tag plus body. All 16,963 chunks passed CRC validation and the
scan ended exactly at EOF. There were 16,185 `ETRY` chunks.

Each selected `ETRY` began with a big-endian u32 path length and that many
path bytes, without a NUL terminator in these selected paths. The remaining
data decoded as
`>I4s20s20s4sIII` followed by the compressed bytes: operation count, operation,
old SHA1, new SHA1, compression, compressed length, destination offset and raw
length. The selected entries each contained one `A` operation, zero old SHA1,
`Z` compression and zero destination offset. Zlib output matched the declared
length and new SHA1, with no unused compressed tail. No earlier installation
or binary-delta application was needed for these complete added files.

All offsets below are absolute in the uncompressed patch, not its outer ZIP.

| Resource / original path | ETRY chunk | Zlib stream | Compressed bytes | Output bytes / SHA-256 |
| --- | --- | --- | ---: | --- |
| Gridania `data/29/B0/00/20.DAT` | `0x1941FA47` | `0x1941FAA7` | 25,121 | 132,496 / `c0d075faed8baa650756e1d27b8550d242d200c9a980855f453fc88e15ae7887` |
| Limsa `data/29/D9/00/1F.DAT` | `0x196A563A` | `0x196A569A` | 22,435 | 120,912 / `13e4c6b12da2b0cd34063e13e8910bb3473c185a5843f02fa5fb9954f83e0add` |
| Ul'dah `data/61/5A/00/23.DAT` | `0x19FC634D` | `0x19FC63AD` | 21,769 | 123,232 / `aae0a9f6d5506c6468192c90b8fb1c0b6c8d4dbf4ab628cc13594a4cba759ef9` |

The original Gridania output SHA1 was
`4519095b0cbc9b24b0d4a8763e6730eb4d9ab000`. Limsa and Ul'dah's recovered
outputs matched their final-client counterparts in the
[seasonal resource finding](city-seasonal-weather-selectors.md#weather-payload-reuse).
Gridania's original output differed from its final Xmas-marked resource.
This comparison authenticates an October candidate, not the separately
refreshed December Gridania version.

## Layout and retained dependencies

`XIVLegacy/xivl-tools:tools/research/census_layout.py`, revision
`75fe7414bf3f1ecd779825083d477adc4ed5f319`, SHA-256
`5afccbd58e7faa491f860ce51e59a8c65a4a9d1850931e38f577d2fc3390e5e3`,
`parse_layout`, parsed the extracted original. Python `struct` independently
read the outer rows, and `hashlib` checked the explicit numeric target paths.
No client or server ran.

The output began with `MapLayoutResourceData`, version `1.1.0` at `+0x18`.
Its 30 rows at `0x40`, stride `0x20`, ended exactly at scene offset `0x400`.
The [outer-table framing](city-weather-direct-inputs.md#sources-and-table-framing)
defines name, tag and key fields. Row zero was empty-tag `DEF_BLK`, key zero.
The other 29 rows named 29 distinct nonzero keys: four `btm`, fourteen `bxt`
and eleven `fcss`. All were readable in the supplied final install.
The [input inventory](gridania-halloween-weather-inputs.csv) preserves every
row, offset, target path, length and digest, including the empty row.

Gridania clear weather `0x29B0000F` referenced a subset of 27 of these keys.
The two additional Halloween targets were complete added files in the same
October patch. Direct extraction and full-byte comparison found them unchanged
in the final install:

| Original layout row / ID field | Target / tag / observed name | October ETRY / zlib offset | Bytes / SHA-256 |
| --- | --- | --- | --- |
| 11 / `0x1B4` | `0x72E8006F` / `bxt` / `2TiVnFf0f_hall_` | `0x1A039CBA` / `0x1A039D1A` | 524,820 / `be373f22ba8e82e691fe802371d2b299bb419be26462eb6049f7761c235da6e6` |
| 29 / `0x3F4` | `0x5B13008C` / `fcss` / `0ZgMJIforesthal` | `0x19B9E5A3` / `0x19B9E603` | 134,336 / `218d29701359d06a35f407f5f10618759746f593b6553bc84f6e8fd85229be90` |

The targets began with `SEDBtxb` and `SEDBSSCF`, respectively. The layout
contained `sdef_hallo_imp` strings at file `0x1074` and `0x8D6C`, and no
`cbind_xmas` or `vfx_cam_xmas` strings. Its typed named objects included
DrawEnv, Fog and SE classes. No typed named VFXBaseObject was recovered by
that scan. The final Gridania layout instead contained typed `cbind_xmas`
InstanceObject at file `0x6460` and `vfx_cam_xmas` VFXBaseObject at `0x8A58`.
Names and types establish the resource difference, not a rendered image.

This was an outer-table and selected original-byte check. It did not interpret
every SSCF table record, prove transitive dependency closure for every format,
or qualify every resource's native instantiation. No missing outer target
required an additional overlay file in this install. Runtime compatibility
still needs the concrete client witness below.

## Why the weather ID must remain separate from the bytes

The [three-city selection contract](../net/city-weather-selection.md) owns
`ffxivgame.exe` VA `0x00624180`, region child lookup at `0x0079B050` and
queue construction at `0x007E80F0`. Its pinned retail executable and Ghidra
observations establish the numeric ID-to-resource path. In the original final
region table, root 103's type-0 weather 8027 row at `0xDF0` selected
`0x29B00020`, token `wtr_hall`. Roots 106, 115, 122 and 123 also selected this
same key for 8027, at rows `0x1E10`, `0x3700`, `0x53B0` and `0x59E0`.
A replacement at that path affects all these aliases, not only one city zone.

The [TimeLine evaluator](timeline-weather-condition.md#queue-id-and-first-matching-entry) owns
VA `0x007FCBF0` and `0x007E14D0`: conditions consumed the weather queue's
recovered ID. The final six city layouts' Halloween masks selected 8027, as
bound in the [seasonal selector finding](city-seasonal-weather-selectors.md).
Changing atmosphere by selecting a different ordinary weather ID also changes
that established condition input. Sending two IDs does not establish two
independent persistent owners for weather and decoration.

Replacing only the payload at the existing key preserves the final region
lookup and the established 8027 condition input. This is the static basis for
the restoration candidate. It does not require a custom weather ID, a region
table append, old city-layout files, or a second scheduler bridge. The separate
[sky/environment path](weather-sky-fog-environment-path.md) and
[seasonal work plane](../event/seasonal-control-plane.md) do not establish an
independent command that repairs this particular weather payload.

## Restoration and remaining discriminator

Use the authenticated October Gridania bytes at the original numeric path
through an optional file overlay. Keep the installed original intact and keep
the final region table, resident city layouts, Limsa/Ul'dah weather resources
and current 8032 payloads. The candidate contains one replacement DAT. The
two Halloween-specific targets above already exist with identical bytes.

A fresh client session must witness ID 8027 selecting the replacement in both
Gridania public/interior zones, with the intended atmosphere and resident
Halloween decorations together. Check arrival, activation while resident,
crossing, reconnect, event off, and disabling the overlay in another fresh
session. Confirm no higher-priority override masked the selected weather.
Check alias scope and Limsa/Ul'dah controls. Until that witness exists, the
result is an authenticated original resource and a statically supported
restoration candidate, not a verified visual fix or a retail server schedule.
Ordinary climate probabilities, local boundaries and seasonal server timing
were not established by these resources.
