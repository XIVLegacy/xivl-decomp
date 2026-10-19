# City interior winter-family owner joins

The five retained Starlight-family show/hide scheduler pairs have exact
serialized property, vector, owner and resident-resource joins in the
final 1.23b interiors. Each show vector contains 23 authored entries and
selects none; each hide vector selects all 23. An empty selected set is
not a zero entry count or a null property. These final masks do not
establish an active 8032 Starlight switch.

The [historical layout delta](city-seasonal-weather-selectors.md#historical-layout-delta)
owns the family classification and earlier 8032 selector counts. The
final-client joins here do not authenticate historical per-pair joins or
transfer that historical selector to the final resources.

## Sources and typed references

The exact three layout source pins, sizes, hashes and payload bases are in
[Original resources and method](city-interior-halloween-owner-joins.md#original-resources-and-method):
Gridania layout 331 / `0x29B00002`, Limsa Lominsa layout 131 /
`0x29D90002`, and Ul'dah layout 431 / `0x615A0003`. Only those three
original DATs were read. No resident resource leaf was opened.

The producing parser was `xivl-tools:tools/research/census_layout.py`,
`parse_layout`, revision `75fe7414bf3f1ecd779825083d477adc4ed5f319`,
SHA-256 `5afccbd58e7faa491f860ce51e59a8c65a4a9d1850931e38f577d2fc3390e5e3`,
with Python `struct` for bounded SCB records. The declared type shapes,
type-table row locators and reference conventions are the same as those
verified in the [interior typed contract](city-interior-halloween-owner-joins.md#typed-selector-and-owner-contract).
The format references are `xivl-tools:docs/formats/world-layout.md`,
Node declarations and Fields consumed by the zone reader, and
`xivl-tools:docs/formats/sedb-res.md`, Resource ID to DAT path and SEDB
container header. Every selected reference and complete declared node extent was
checked in the original final-client bytes.

Offsets below are absolute from DAT start; references resolve relative
to the city's lyb payload base. Each selected TimeLine is a declared
`0x28`-byte node. Its `+0x1C` reference reaches a declared `0x18`-byte
property named `wprp_0001`; property `+0x10/+0x14` supplies a reference
and count for 23 declared eight-byte entries with no reference descriptors.
Each complete `0xB8`-byte vector run fits its declared extent.

The IDs are 8001-8017 and then 8027-8032, in that order. The first
u32's low half holds the ID. Selected entries have high half `0xFFFF`
and second u32 `0xFFFFFFFF`; unselected entries have both fields zero.
Every show entry uses the unselected encoding, and every hide entry
uses the selected encoding, including 8032. That encoding alone does
not establish which scheduler runs or what becomes visible.

## Exact timeline and scheduler joins

Timeline names are `time_bg_<suffix>_show` and
`time_bg_<suffix>_hide`. Every cell lists show / hide offsets.

| City | Suffix | TimeLine | Property | Vector | Embedded SCB |
| --- | --- | --- | --- | --- | --- |
| Gridania | `crs1` | `0x64DA0` / `0x64DC8` | `0x67870` / `0x67888` | `0x68700` / `0x687B8` | `0x6FFF0` / `0x70320` |
| Limsa Lominsa | `itm0_pstr1_h` | `0x8A314` / `0x8A33C` | `0x92CF0` / `0x92D08` | `0x94170` / `0x94228` | `0xB12D0` / `0xB1600` |
| Limsa Lominsa | `itm0_pstr3_h` | `0x8A364` / `0x8A38C` | `0x92D20` / `0x92D38` | `0x942E0` / `0x94398` | `0xB1930` / `0xB1C60` |
| Ul'dah | `xmas1` | `0x6BF30` / `0x6BF58` | `0x6F378` / `0x6F390` | `0x6FDE8` / `0x6FEA0` | `0x7FD30` / `0x80060` |
| Ul'dah | `xmas2` | `0x6BF80` / `0x6BFA8` | `0x6F3A8` / `0x6F3C0` | `0x6FF58` / `0x70010` | `0x80390` / `0x806C0` |

TimeLine `+0x14/+0x18` declares the embedded SCB reference and byte size.
All ten selected spans are observed `0x330`-byte SCBs with `0x30`-byte
SEDB headers and matching total sizes. The
[generic SCB framing](cut-scheduler-clip-layout.md#resource-header-and-directory)
resolves the `@CATT` and `@CCPT` class-key entries through the bounded
String table. Each has one `LayUnitMemberActor` record at chunk
`+0xD0`, size `0x24`, class ordinal 0, and one `ShowHideClip` record
at chunk `+0x1E0`, size `0x14`, class ordinal 0 and actor ordinal 0.
The clip occurs in the second of two framed blocks; the first has no
clip. Every pair differs at exactly chunk `+0x1F0`: show is 1 and hide
is 0. These are serialized class and flag observations, not a native
receiver or visible-hide trace.

## Exact owners and resident resource mappings

Every timeline has one observed direct UnitTree owner, shared by its
show/hide partner. These matches were enumerated from declared child
references, preserving aliases before counting. The UnitTrees are
`0x50`-byte nodes; children are the declared five-descriptor `0x30`
shape and use `+0x18` for their base reference. Every timeline child
alias is exactly `show` or `hide` in these selected inputs.

| City / suffix | UnitTree / name | Timeline children show / hide | Resident group / child | BGParts |
| --- | --- | --- | --- | --- |
| Gridania / `crs1` | `0x2A7E0` / `sgrp_w_itm0_pstr1_h` | `0x2FC30` / `0x2FC60` | direct / `0x2FC90` | `0x627B4` |
| Limsa / `itm0_pstr1_h` | `0x34A00` / `sgrp_w_itm0_pstr1_h` | `0x3C150` / `0x3C180` | `0x7AA10` / `0x3C120` | `0x87044` |
| Limsa / `itm0_pstr3_h` | `0x34A50` / `sgrp_w_itm0_pstr3_h` | `0x3C1E0` / `0x3C210` | `0x7AA40` / `0x3C1B0` | `0x87098` |
| Ul'dah / `xmas1` | `0x27B70` / `sgrp_w_itm0_pstr1_h` | `0x2D450` / `0x2D480` | direct / `0x2D420` | `0x6A260` |
| Ul'dah / `xmas2` | `0x27BC0` / `sgrp_w_itm0_pstr2_h` | `0x2D4E0` / `0x2D510` | direct / `0x2D4B0` | `0x6A2B4` |

Gridania's owner has three direct children beginning at `0x2FC30` and
zero group reference/count. Ul'dah's two owners have three direct
children beginning at `0x2D420/0x2D4B0` and zero group reference/count.
Each Limsa owner has two direct timeline children and one separate
five-descriptor `0x30`-byte group: `grp_0213` at `0x7AA10` and
`grp_0215` at `0x7AA40`. Group `+0x20/+0x24` gives its one-resident
child run at `0x3C120/0x3C1B0`; both subgroup reference/count fields
at `+0x14/+0x18` are zero.

Every resident base is a declared `0x54`-byte BGParts. Its `+0x20`
string reference reaches a resource key that matches one exact outer
`brt` row. Resource-table row `+0x10` holds the tag and `+0x14` holds
the resource ID. The five resident observations resolve to three
distinct resource IDs; the shared key and ID are preserved below.

| City / suffix | BGParts name | Key-reference field / string | Resource key | Outer `brt` row | Resource ID |
| --- | --- | --- | --- | --- | --- |
| Gridania / `crs1` | `w_itm0_pstr1_h` | `0x627D4` / `0x17C4B` | `06oR8Xw_itm0__h` | `0x3880` | `0x897A013C` |
| Limsa / `itm0_pstr1_h` | `w_itm0_pstr1_h` | `0x87064` / `0x1C6A4` | `06oR8Xw_itm0__h` | `0x5380` | `0x897A013C` |
| Limsa / `itm0_pstr3_h` | `w_itm0_pstr3_h` | `0x870B8` / `0x1C6C4` | `19WxTFw_itm0__h` | `0x46A0` | `0x897A0141` |
| Ul'dah / `xmas1` | `w_itm0_pstr1_h` | `0x6A280` / `0x14A26` | `06oR8Xw_itm0__h` | `0x4100` | `0x897A013C` |
| Ul'dah / `xmas2` | `w_itm0_pstr2_h` | `0x6A2D4` / `0x14A46` | `4E0jq7w_itm0__h` | `0x1DE0` | `0x897A0140` |

These IDs map to `data/89/7A/01/3C.DAT`,
`data/89/7A/01/41.DAT` and `data/89/7A/01/40.DAT`. They identify
table targets only. Their availability, bytes, appearance, dependencies
and portable model variants were not inspected.

## Evidence boundary and reproduction

This establishes final-client serialized joins for the five selected
pairs. It leaves the serialized actor/member-token to active native
receiver edge unresolved and does not prove native clip effects,
rendering, per-instance placement, an 8032 event switch, runtime
condition construction, backend timing, a server calendar or policy.
The historical 8032 counts and the separate proposed 8071 overlay
candidate retain their own evidence limits. No historical patch bytes
were read for this finding.

To reproduce, verify the linked three original source pins and parser
identity. Follow each named TimeLine's `+0x1C` property reference and
property `+0x10/+0x14` vector run; check all 23 IDs and both encoding
words per entry. Follow each TimeLine's complete embedded SCB span and
generic class/actor/clip framing. Enumerate all direct UnitTree child
matches, then the separate Limsa group runs, BGParts key strings and
all matching outer resource rows. Count only after retaining those
matches and aliases.
