# Limsa seasonal weather extension boundary

Limsa's final 1.23b public layout directly connects Halloween weather 8027
selectors to resident decoration scheduler pairs. A new weather ID can pass
the inspected region registry, but its constructed condition-view and backend
bindings remain unresolved. Sharing an existing weather resource key also
aliases the native queue's recovered ID. These are separate requirements for
an additive event.

## Original resources and method

The selected public layout is layout 121, `data/29/D9/00/01.DAT`, resource key
`0x29D90001`, 1,197,104 bytes, SHA-256
`bc40d7ff57ab8998978532cce4564e2488ed81d6d10b8f7ede3e1586b0b26523`.
Its `SEDBlyb` begins at file `0x99A0`; serialized references P resolve to
file `0x99D0 + P`. Offsets below are file offsets unless marked otherwise.

Python `struct` followed the declared node extents and reference descriptors,
using `xivl-tools:tools/research/census_layout.py`, `parse_layout`, at revision
`75fe7414bf3f1ecd779825083d477adc4ed5f319`, script SHA-256
`5afccbd58e7faa491f860ce51e59a8c65a4a9d1850931e38f577d2fc3390e5e3`.
The field interpretation is
documented in `xivl-tools:docs/formats/world-layout.md`, Node declarations,
Declared shapes and Fields consumed by the zone reader. This is a typed
reference join, rather than a join by adjacent names or matching counts.
Embedded scheduler spans were checked against their sizes, class labels and
paired byte differences. No scene playback was observed.

## Halloween selector and resident group

Each declared `BaseObjects/TimeLine/TimeLineBaseObject` has a reference at
serialized `+0x1C` to a `0x18`-byte property named `wprp_0001`. Property
`+0x10` points to its weather vector, and `+0x14` is the entry count, 23.
The show vectors select only 8027. Their paired hide vectors select the
other 22 entries, covering 8001-8017 and 8028-8032. The entry encoding is in
[City seasonal weather selectors](city-seasonal-weather-selectors.md#corpus-boundary).
No entry describes an added ID N.

| Timeline suffix in `time_bg_hw_` | Show timeline | Show property | Show vector | Show SCB |
| --- | --- | --- | --- | --- |
| `obj1_h_show` | `0xEC034` | `0xFF3D8` | `0x100E98` | `0x120250` |
| `obj2_h_show` | `0xEC084` | `0xFF408` | `0x101008` | `0x120930` |
| `obj3_h_show` | `0xEC0D4` | `0xFF438` | `0x101178` | `0x121010` |
| `obj4_h_show` | `0xEC124` | `0xFF468` | `0x1012E8` | `0x1216F0` |
| `obj5_h_show` | `0xEC174` | `0xFF498` | `0x101458` | `0x121DD0` |
| `obj6_h_show` | `0xEC1C4` | `0xFF4C8` | `0x1015C8` | `0x1224B0` |
| `obj7_h_show` | `0xEC214` | `0xFF4F8` | `0x101738` | `0x122B90` |
| `obj8_h_show` | `0xEC264` | `0xFF528` | `0x1018A8` | `0x123270` |

For each row, the hide timeline, property, vector and SCB are respectively
`+0x28`, `+0x18`, `+0xB8` and `+0x370` from the listed show locations.
Each SCB is `0x370` bytes and contains `LayUnitMemberActor` and `ShowHideClip`
class labels. Each pair differs only at chunk `+0x220` and `+0x234`, with
show bytes 1 and hide bytes 0.
The first show/hide SCB SHA-256 digests are
`c6ee7bfefdf2f354ef9a73c2c1e6cc8bf8f4b9f528b7370ad9890af67dae6884`
and `b34383be50bf85e6397289e6cced956d65cd9febf8092bad8391397c833601ee`.

The first pair has a concrete resident owner:

1. `sgrp_bg_hw_obj1_h` at `0x496D0` is a declared UnitTree. Its `+0x30/+0x34`
   references two child records at `0x59380/0x593B0`, named `show` and `hide`.
   Child `+0x18` references the first two timelines above.
2. The same UnitTree's `+0x24/+0x28` references one group at `0xC25E0`,
   named `grp_0274`. Group `+0x20/+0x24` references two resident children
   at `0x59320/0x59350`.
3. Those child `+0x18` references lead to the BGPartsBaseObjects below.
   Their `+0x20` strings join exactly to the outer resource table's `brt` rows.

| Resident child | BGParts node / name | Resource-table row / name | Resource key |
| --- | --- | --- | --- |
| `s0t0_hw_obj1_h_001` | `0xDC41C` / `s0t0_hw_obj1_h` | `0x20E0` / `18KWcss0t0_hw_h` | `0x899404E0` |
| `s0t0_a0_tal1_s_004` | `0xDAF70` / `s0t0_a0_tal1_s` | `0x17C0` / `0bgOLQs0t0_a0_t` | `0x89940598` |

The current resources at those keys are 48,982 and 11,510 bytes, SHA-256
`24f6577bacd69cb65bbe1d2a4589d8711cbf68f37c2bf32dbc3c4d5d32167cdb`
and `ee1edb4052bba8773216cbd18a4e1aa32112d894a5f50d16e5793ebdbc33bb9c`.
This establishes resident BGParts membership and available resource inputs.
It does not identify a portable `b###` appearance variant, prove a mesh's
visual identity, or join every rendered descendant to the clip receiver.

All eight scheduler pairs have concrete resident membership in this layout.
Each pair shares one declared `RefObjects/UnitTree/UnitTreeObject`, size
`0x50`. UnitTree `+0x30/+0x34` gives two `0x30`-byte child records;
their `+0x18` references lead to the show and hide timelines above.
UnitTree `+0x24/+0x28` gives one `0x30`-byte group. Each group's
`+0x14/+0x18` sub-group reference/count is zero, and its `+0x20/+0x24`
gives two resident child records. These resident runs are disjoint from
their owner's timeline-child run.

Names below were read after the reference joins. The UnitTree names are
`sgrp_bg_hw_obj1_h` through `sgrp_bg_hw_obj8_h`, in the listed order.

| Pair in `time_bg_hw_` | UnitTree | Timeline children show / hide | Group / name | Resident children |
| --- | --- | --- | --- | --- |
| `obj1_h` | `0x496D0` | `0x59380` / `0x593B0` | `0xC25E0` / `grp_0274` | `0x59320` / `0x59350` |
| `obj2_h` | `0x49720` | `0x59440` / `0x59470` | `0xC2610` / `grp_0275` | `0x593E0` / `0x59410` |
| `obj3_h` | `0x49770` | `0x59500` / `0x59530` | `0xC2640` / `grp_0281` | `0x594A0` / `0x594D0` |
| `obj4_h` | `0x497C0` | `0x595C0` / `0x595F0` | `0xC2670` / `grp_0280` | `0x59560` / `0x59590` |
| `obj5_h` | `0x49810` | `0x59680` / `0x596B0` | `0xC26A0` / `grp_0279` | `0x59620` / `0x59650` |
| `obj6_h` | `0x49860` | `0x59740` / `0x59770` | `0xC26D0` / `grp_0278` | `0x596E0` / `0x59710` |
| `obj7_h` | `0x498B0` | `0x59800` / `0x59830` | `0xC2700` / `grp_0277` | `0x597A0` / `0x597D0` |
| `obj8_h` | `0x49900` | `0x598C0` / `0x598F0` | `0xC2730` / `grp_0276` | `0x59860` / `0x59890` |

Every resident child's `+0x18` references a declared `0x54`-byte
`BaseObjects/BG/BGPartsBaseObject`. BGParts `+0x20` references the exact
key string in the resource-table column below. Each string matches exactly
one outer `brt` row; row `+0x14` supplies the resource ID. All 16 child
records are distinct. Their base references reach 11 distinct BGParts
nodes and 11 distinct resource rows/IDs. The repeated `0xDAF70` base is
shared by six children, in `obj1_h` through `obj5_h` and `obj8_h`.

| Pair | Resident child / name | BGParts / name | Resource-table row / name | Resource ID |
| --- | --- | --- | --- | --- |
| `obj1_h` | `0x59320` / `s0t0_hw_obj1_h_001` | `0xDC41C` / `s0t0_hw_obj1_h` | `0x20E0` / `18KWcss0t0_hw_h` | `0x899404E0` |
| `obj1_h` | `0x59350` / `s0t0_a0_tal1_s_004` | `0xDAF70` / `s0t0_a0_tal1_s` | `0x17C0` / `0bgOLQs0t0_a0_t` | `0x89940598` |
| `obj2_h` | `0x593E0` / `s0t0_hw_obj2_h_001` | `0xDC470` / `s0t0_hw_obj2_h` | `0x17E0` / `18KWcbs0t0_hw_h` | `0x899404E1` |
| `obj2_h` | `0x59410` / `s0t0_a0_tal1_s_002` | `0xDAF70` / `s0t0_a0_tal1_s` | `0x17C0` / `0bgOLQs0t0_a0_t` | `0x89940598` |
| `obj3_h` | `0x594A0` / `s0t0_hw_obj3_h_001` | `0xDC4C4` / `s0t0_hw_obj3_h` | `0x1A00` / `18KWbEs0t0_hw_h` | `0x899404E2` |
| `obj3_h` | `0x594D0` / `s0t0_a0_tal1_s_004` | `0xDAF70` / `s0t0_a0_tal1_s` | `0x17C0` / `0bgOLQs0t0_a0_t` | `0x89940598` |
| `obj4_h` | `0x59560` / `s0t0_hw_obj4_h_001` | `0xDC518` / `s0t0_hw_obj4_h` | `0x2EE0` / `18KWffs0t0_hw_h` | `0x899404E3` |
| `obj4_h` | `0x59590` / `s0t0_a0_tal1_s_004` | `0xDAF70` / `s0t0_a0_tal1_s` | `0x17C0` / `0bgOLQs0t0_a0_t` | `0x89940598` |
| `obj5_h` | `0x59620` / `s0t0_hw_obj5_h_001` | `0xDC56C` / `s0t0_hw_obj5_h` | `0x17A0` / `18KWeIs0t0_hw_h` | `0x899404E4` |
| `obj5_h` | `0x59650` / `s0t0_a0_tal1_s_004` | `0xDAF70` / `s0t0_a0_tal1_s` | `0x17C0` / `0bgOLQs0t0_a0_t` | `0x89940598` |
| `obj6_h` | `0x596E0` / `s0t0_hw_obj6_h_001` | `0xDC5C0` / `s0t0_hw_obj6_h` | `0x1760` / `18KWebs0t0_hw_h` | `0x899404E5` |
| `obj6_h` | `0x59710` / `s0t0_hw_obj6_s_004` | `0xDC614` / `s0t0_hw_obj6_s` | `0x1780` / `45caGls0t0_hw_o` | `0x899404E6` |
| `obj7_h` | `0x597A0` / `s0t0_hw_obj7_h_001` | `0xDC668` / `s0t0_hw_obj7_h` | `0x2FA0` / `18KWdEs0t0_hw_h` | `0x899404E7` |
| `obj7_h` | `0x597D0` / `s0t0_hw_obj7_s_004` | `0xDC6BC` / `s0t0_hw_obj7_s` | `0x2FC0` / `45caFOs0t0_hw_o` | `0x899404E8` |
| `obj8_h` | `0x59860` / `s0t0_hw_obj8_h_001` | `0xDC710` / `s0t0_hw_obj8_h` | `0x29C0` / `18KWhfs0t0_hw_h` | `0x899404E9` |
| `obj8_h` | `0x59890` / `s0t0_a0_tal1_s_004` | `0xDAF70` / `s0t0_a0_tal1_s` | `0x17C0` / `0bgOLQs0t0_a0_t` | `0x89940598` |

These are serialized membership and typed resource-table mappings. The
resource-ID-to-path convention does not prove that every target file exists.
This membership check read the layout only; it did not inspect additional
resident resources or their dependencies. It does not identify portable
`b###` appearance variants, establish visual identity, or identify the native
clip receiver for every descendant. The separate decoration-condition
boundary below remains unresolved.

## Additional IDs and resource-key aliases

Native addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Retained Ghidra listings were checked against original PE bytes using pefile
2024.8.26 and Capstone 5.0.7 in x86-32 mode.

The RegionResourceData child loop at `0x0079DB01..0x0079DB5A` and child
constructor `0x0079A280` impose no numeric weather-ID range. The constructor
copies row `+0x00/+0x04/+0x08/+0x0C` to child
`+0x58/+0x5C/+0x60/+0x64`. Normalizer `0x0079B190` accepts an exact
supplied ID among type-zero children. A miss falls back to the first nonzero
type-zero child ID. `0x0079B050` performs the corresponding ID-to-resource
lookup. This establishes table-driven admission in those bodies, subject to
the [packet's unsigned 16-bit ID](../net/weather-transition-runtime.md).
It does not establish support in every downstream weather consumer.

Resource identity and weather ID are separate. `0x0079ADF0` returns the first
child ID whose `+0x60` equals the supplied resource key, without a type filter.
`0x0079B540` tries the preferred region through `0x0079B310`, then other roots
if the result is zero. The [weather queue](../net/weather-transition-runtime.md#resource-queue-behavior)
stores that recovered ID and deduplicates by resource key.

If a new ID N shares Limsa's Halloween key and the existing 8027 child remains
the first matching row, the actor can retain normalized ID N while the queue
recovers 8027. An existing queue entry can also prevent another insertion.
A distinct key K_NEW avoids this particular alias and queue collision, subject
to valid resource construction. Reordering old rows would change existing
reverse mappings and is not a neutral preservation strategy.

## Decoration-condition boundary

The [TimeLine weather-condition evaluator](timeline-weather-condition.md)
owns the native attachment, queue-ID operand, data-derived count, first-match
rule, absent-ID reset and state-dependent dispatch. It distinguishes the
property-interface gate from evaluator state and preserves the separate
getter chain used by the matched-zero-bit reset veto.

The remaining join is retained TimeLine record -> constructed record view ->
valid condition inputs and backend receivers -> concrete scheduler/clip
invocation. These construction and receiver bindings remain unresolved.
Consequently neither automatic activation by N nor sufficiency of appending
N to a vector is established. The serialized 8027 owner join does not prove
live playback.

## Minimum additive resource requirements

The existing 8027 route uses the final Limsa binding `0x29D9001F` and the
resident selectors above. Its resource/marker identity is in
[City seasonal weather selectors](city-seasonal-weather-selectors.md#weather-payload-reuse).
Those inputs do not require allocating a new weather ID merely to retain the
existing Halloween conditions.

An additive ID instead requires all of the following:

- A nonzero u16 N, a type-zero child in Limsa region root 101, and a distinct
  resolvable resource key K_NEW. No numeric allocation is made here.
- A valid rebuilt `RegionResourceData`: insert the `0x30`-byte row inside the
  selected root's child run, update that root's child count and declared table
  byte size, and retain the other root runs. Appending at EOF alone does not
  make a Limsa child. The [loader contract](region-weather-resource-rows.md#native-loader-boundary)
  owns the original table identity and structural fields.
- A compatible weather payload and complete typed dependency inventory.
  Reuse existing keys only where their current bytes are the intended inputs;
  changed dependencies need distinct keys and corrected references.
- A proven condition match for N and valid serialized counts/references for
  any altered resident layout. The native condition boundary above prevents
  claiming that one extra mask entry completes this step.

For Starlight, the pinned historical Limsa weather and December interior
selectors are documented in the [historical layout delta](city-seasonal-weather-selectors.md#historical-layout-delta).
The final 8032 resource is a different presentation, and the final interior's
Starlight show masks are empty. The historical marker and selector evidence
does not enumerate the old wrapper's full dependencies or recover every city
placement. A ready additive Starlight package therefore remains unestablished:
it needs the exact pinned historical payload's typed dependency closure and
the constructed condition-view/backend bindings, while retaining the final
8032 behavior.
