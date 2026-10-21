# City Halloween member keys and context qualification

Three selected public Halloween show schedulers contain four
`LayUnitMemberActor` records whose keys match resident-child IDs in their
declared owners. Limsa Lominsa's selected records use keys 1 and 4;
Gridania and Ul'dah each use key 1. These are typed serialized matches,
without an observed city S/Q/context or successful native lookup.

The native direct-child contracts support a conditional context join for the
Gridania and Ul'dah examples. Limsa's matched residents occupy a separate
group run, whose registration into the callback's context table is a distinct
missing edge.

## Original inputs and method

Python 3.12.0 `struct` decoded selected spans and little-endian fields from
these retail 1.23b files. Original file size and SHA-256, each selected SCB
digest, and its TimeLine reference/size fields matched the existing owner
contracts. Other owner/reference facts were reused from those contracts.

| City / layout | DAT | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| Limsa Lominsa / 121 | `data/29/D9/00/01.DAT` | 1197104 | `bc40d7ff57ab8998978532cce4564e2488ed81d6d10b8f7ede3e1586b0b26523` |
| Gridania / 321 | `data/29/B0/00/01.DAT` | 1300928 | `abc13bd6f096b0fe9703b2092ca4a7f819ff2b88e4d08ae222a9db447f618312` |
| Ul'dah / 421 | `data/61/5A/00/01.DAT` | 1036816 | `69c6ff8488ed9c407a28705cf6cb844d55845f7eed3ad58b31845b11a8331c4c` |

The [Limsa owner contract](limsa-seasonal-weather-extension.md#halloween-selector-and-resident-group)
and [Gridania/Ul'dah owner contract](city-halloween-owner-joins.md#direct-resident-owners)
own the declared node shapes, owner runs, weather selectors and resident
resource joins. This qualification uses one show example per public layout.
It does not extend the member-key result to all public pairs or interiors.

All layout offsets below are absolute file offsets. SCB-relative offsets
refer only to bytes within the selected embedded scheduler, not to native
parsed object Q or scheduler S addresses.

| City / example | UnitTree definition | Show TimeLine | Reference/size fields | SCB offset / bytes | SCB SHA-256 |
| --- | --- | --- | --- | --- | --- |
| Limsa `obj1_h` | `0x496D0` | `0xEC034` | `0xEC048/0xEC04C` | `0x120250 / 0x370` | `c6ee7bfefdf2f354ef9a73c2c1e6cc8bf8f4b9f528b7370ad9890af67dae6884` |
| Gridania `hlw1a` | `0x52270` | `0xFEC2C` | `0xFEC40/0xFEC44` | `0x137D70 / 0x330` | `84ad55ee220f2e727f9d34336af88090ab936bd071c32bdd8e3c1ad18c0957e1` |
| Ul'dah `hw1` | `0x441F0` | `0xCA4E0` | `0xCA4F4/0xCA4F8` | `0xF9580 / 0x330` | `a327c4bb74ad6b0637ac07515fc2ebeb646ab72b60f40b43d5c99f353bbdc187` |

## Serialized member matches

The [SCB layout](cut-scheduler-clip-layout.md#resource-header-and-directory)
defines the directory, string-token resolver, `@CATT` class table and
`@CACT` record iteration. Each selected directory entry 0 is at SCB `+0x60`,
named `schedul`. Its `@CATT`, `@CCPT` and `@CACT` children begin at
`+0x80`, `+0xA0` and `+0xC0`. The class table resolves ordinal 0 to
`LayUnitMemberActor`; every listed actor record uses that ordinal.

Every listed actor record is `0x24` bytes. Its dword `+0x20` is the
[member slot-26 lookup key](../actor/lay-scheduler-member-publication.md#serialized-lookup-key)
when the selected construction/record-retention conditions hold. Resident
child IDs below are their separate serialized `+0x1C` dwords.

| City | Actor ordinal / SCB offset | Actor file offset / key field | Key | Matched resident child / key field | Resident alias |
| --- | --- | --- | ---: | --- | --- |
| Limsa | `0 / 0xD0` | `0x120320 / 0x120340` | 1 | `0x59320 / 0x5933C` | `s0t0_hw_obj1_h_001` |
| Limsa | `1 / 0xF4` | `0x120344 / 0x120364` | 4 | `0x59350 / 0x5936C` | `s0t0_a0_tal1_s_004` |
| Gridania | `0 / 0xD0` | `0x137E40 / 0x137E60` | 1 | `0x5E670 / 0x5E68C` | `f0t0_a0_hlw1a_h_001` |
| Ul'dah | `0 / 0xD0` | `0xF9650 / 0xF9670` | 1 | `0x4EF70 / 0x4EF8C` | `w0t0_h0_hlw1s_h_001` |

The two timeline-child IDs in each selected owner are distinct from these
resident keys. This checks the selected child sets, without a global-key
uniqueness or runtime duplicate-resolution claim.

| City | Show child / ID field / value | Hide child / ID field / value |
| --- | --- | --- |
| Limsa | `0x59380 / 0x5939C / 5` | `0x593B0 / 0x593CC / 6` |
| Gridania | `0x5E6A0 / 0x5E6BC / 4` | `0x5E6D0 / 0x5E6EC / 5` |
| Ul'dah | `0x4EFD0 / 0x4EFEC / 5` | `0x4EFA0 / 0x4EFBC / 4` |

These keys are local lookup operands, not globally unique actor IDs or file
offsets. Key 1's occurrence in all three files does not join their contexts.

## Conditional native composition

The following native contracts were reused without new instruction or PE-data
reads. Their locators are VAs in the pinned retail 1.23b executable identified
by those findings.

| Edge | Published contract | Required identity |
| --- | --- | --- |
| TimeLine to resource R | [Construction context](limsa-fireworks-scheduler-bindings.md#construction-context-boundary), `0x00AB3730` through `0x00AB38B0` | The selected TimeLine's acquired resource must supply the selected SCB span above. |
| R to parsed Q and S | [Parsed-object construction](scb-clip-factory-composition.md#parsed-clip-key-table) and [directory input](scheduler-parser-directory-input.md#selected-parser-input) | Q must derive from that R and remain at this S's `+0x50`; its directory entry 0 supplies parser input D. |
| D to member entry and key | [Member publication](../actor/lay-scheduler-member-publication.md#outer-factory-input-and-scheduler-handoff) and [serialized key](../actor/lay-scheduler-member-publication.md#serialized-lookup-key) | The selected row must construct the same retained member E in this S's collection; slot 26 then supplies the listed key. |
| TimeLine instance to U | [Owner UnitInfo context](timeline-weather-record-view.md#owner-unitinfo-context), `0x007FD830` child setup | The created TimeLine child must retain the inherited context U from the selected parent invocation. This is distinct from its separately created UnitInfo. |
| Child key to lookup result | [Child registration](limsa-fireworks-scheduler-bindings.md#unittree-child-key-table) and [UnitInfo slot 37](timeline-weather-record-view.md#unitinfo-callback-lookup-boundary) | Child `+0x1C` must register the resident interface in the same U's retained table that the callback queries with K. |

For Gridania and Ul'dah, the selected resident and the show TimeLine are
direct children of the same declared UnitTree. The published direct-child
loop consumes serialized UnitTree `+0x30/+0x34`, propagates the inherited
context, and registers child `+0x1C` through U slot 39. If this initializer
uses that selected definition, creates both children and preserves the
published fields/tables, the matching resident key and callback key reach
the same context table. This remains conditional on actual invocation,
successful child admission/construction and retained table entries.

Limsa's resident children instead occur in group `0xC25E0`, reached from
UnitTree `+0x24/+0x28`. Group `+0x20/+0x24` supplies the two-resident run;
the owner's direct child run contains the timelines. The cited direct-child
loop alone does not establish registration of these group members into the
same U table. A group reader/registration path or a concrete selected-state
observation is needed before the numeric matches identify native targets.

## Qualification result and missing witness

The raw record classes, keys, TimeLine spans and selected child IDs have
direct retail-byte support. The S/Q/U composition is conditional, with an
additional group-context edge for Limsa. This qualification's three DAT inputs
and reused native contracts supply no observed S/Q/U invocation.

A selected-state join needs evidence for all of these identities:

1. The created TimeLine instance and its parent invocation use the selected
   owner/child definition. A definition may have multiple placed instances.
2. The acquired resource R supplies the pinned embedded SCB, and the parsed Q,
   retained S and member record derive from that same resource invocation.
   Serialized SCB `+0x60` is not native `Q+0x60`.
3. The TimeLine callback's inherited U and its lookup table retain the resident
   registration for the listed K. Limsa also requires the group-member join.

The current resource-acquisition locator is `0x00AB38B0` in the published
TimeLine path; that resource result has not been byte-bound to one of these
selected SCBs by this qualification. The group-field reader/registration edge
above is separately missing. Neither gap is resolved by matching a name, a
class label, a small numeric key or a file offset to an object address.

The [three-city activation limits](../actor/lay-scheduler-member-construction.md#three-city-activation-boundary)
retain the separate ShowHideClip operation-provider/indexed-resident and
Ul'dah material requirements. No historical activation, complete decoration
coverage, visible identity, effect playback or server input is established.

## Reproduction

Authenticate the three original DATs and selected SCB spans against the pins
above. TimeLine reference values resolve with payload bases `0x99D0` for
Limsa, `0xD5B0` for Gridania and `0x90B0` for Ul'dah. Read only each
selected TimeLine's reference/size pair, selected show SCB and the ten listed
child-ID dwords.

Follow the documented SCB directory and string-token rules. Resolve `@CATT`
before assigning each record's class, then follow signed record-size links for
the `@CACT` count. Read member `+0x20` and compare it with the selected
resident child `+0x1C`; check the two timeline IDs separately. This uses
2512 SCB bytes, 24 reference/size bytes and 40 child-key bytes: 2576 distinct
payload bytes across 16 selected reads. Whole-file SHA-256 authentication is
separate. Reuse the typed owner and native contracts without re-extracting
their complete inventories or bodies. No resident leaf, interior, historical
layout, PE payload or live client state is required for the serialized matches.
