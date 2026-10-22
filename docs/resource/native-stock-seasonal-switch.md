# Native seasonal switch using existing DATs

The retail client supplied native interfaces for selecting an existing
weather resource and evaluating resident decoration conditions separately.
The concrete profile below combines those interfaces without replacing DATs.
It is a client-extension design derived from static evidence, not a working
unchanged-client packet sequence or an observed seasonal replay.

The profile deliberately reuses a cross-city Halloween atmosphere and the
retained Gridania Xmas atmosphere. It does not authenticate exact original
Gridania Halloween lighting or the recovered historical all-city Starlight
composition. The [stock component finding](stock-seasonal-component-bindings.md)
owns those resource and compatibility boundaries.

## Evidence identity and method

Native locators are VAs in retail 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, image base `0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Retained Ghidra instruction listings, Python 3.12.0, pefile 2024.8.26 and
Capstone 5.0.7 x86-32 established the cited instructions and method cells.
These observations did not execute a client or install a hook.

The [weather catalog](city-weather-catalog.csv) pins the existing file paths,
sizes, hashes and registry rows. The original `RegionResourceData` file
`data/03/C0/00/00.DAT`, 52,336 bytes, had SHA-256
`c04b0d998aea4c1b13ed322292a5aa5af45485c698da2315171c3c024bcb9a74`.
Its [declared row runs](native-seasonal-resource-selection.md#original-registry-and-seasonal-paths)
and the native reverse lookup established that all three keys below
recovered weather ID 8027, including cross-root fallback. Stock had no
8033 registration.

## Concrete existing-file profiles

These are proposed client-extension choices, not additional retail mappings:

| Profile input | Current city root | Requested existing weather key | Numeric DAT path |
| --- | --- | --- | --- |
| Halloween, 8027 | Limsa 101 or Gridania 103 | `0x29D9001F` | `data/29/D9/00/1F.DAT` |
| Halloween, 8027 | Ul'dah 104 | `0x615A0023` | `data/61/5A/00/23.DAT` |
| Christmas, 8033 | 101, 103 or 104 | `0x29B00020` | `data/29/B0/00/20.DAT` |
| Ordinary mode | Any | Original client selection | Original path |

The input 8033 would be consumed by the extension before ordinary ID
normalization. It would not be passed unchanged to the stock ID setter.
The extension would submit the chosen key through the native selector-`0x0D`
contract or its qualified ready-state call. That route accepted one u32 key
at `0x00624240`, reverse-resolved it at `0x00624280`, and queued it at
`0x0062429A`. This is distinct from packet opcode `0x000D` and the
selector-`0x0C` u16-ID route. The
[retained-state contract](../net/weather-transition-runtime.md#direct-resource-key-requests-and-retained-state)
owns readiness, queue deduplication and clearing.

For any other input, including 8032, the proposed extension would return to
ordinary mode and delegate the original request. It would not repurpose the
stock 8032 resources. Region aliases need the same explicit root classification
as their authenticated registry rows; the table does not classify every zone.

### Cross-city DrawEnv selection

For Gridania Halloween, a raw key substitution alone leaves an identified
gap: Limsa's weather wrapper has no `dwev_01`, while Gridania's town and
field can select that zone DrawEnv. The
[typed missing-name joins](stock-seasonal-component-bindings.md#cross-city-and-starlight-compositions)
own the file hashes and node locators.

The proposed profile therefore includes one explicit compatibility policy:
on that Gridania Halloween spatial-weather lookup only, resolve a missing
`dwev_01` using the chosen weather's `dwev_00`. Native `0x007DCDF0`
called the exact-name lookup `0x007E4630` at `0x007DCEF5`. The latter
returned the hit's node+`0x64` value at `0x007E4677`, or zero on an
ordinary miss. Nonzero replaced the spatial holder's current DrawEnv at
`0x007DCF11`; zero retained the zone selection. Thus the policy has a
specific lookup boundary. It is an added alias policy, not a discovered
stock fallback or proof of original lane-01 lighting equivalence.

Other names retain their original exact-name behavior. Actual cross-city
environment, sky, sound, camera and render admission still require a
qualified client witness. A shared format and successful key lookup alone
do not establish those effects.

## Resident condition selection

The weather queue would still recover ID 8027 for either seasonal profile.
Changing only the resource therefore leaves the original Halloween masks
selected. The extension must separately select the intended resident family.
This design uses the native condition result rather than accumulating manual
show requests against the ordinary weather evaluator.

At `0x007FCCE7`, `0x007FCBF0` normalized the matched entry's selected bit
into BL. The next instruction, `0x007FCCEA`, compared BL with complete
property C+`0x46`. A change stored the new bit there and its complement at
C+`0x45`. The
[existing state machine](timeline-weather-condition.md#selected-bit-and-conditional-dispatch)
then requested activation through `0x007FCA30` or reset through
`0x007FCAD0`, with its existing backend, helper, veto and return conditions.
This provides a precise candidate interception point before that comparison.

For identified seasonal properties, the extension would supply:

| Property role | Halloween profile | Christmas profile |
| --- | ---: | ---: |
| Halloween show | 1 | 0 |
| Halloween hide | 0 | 1 |
| Retained winter show | 0 | 1 |
| Retained winter hide | 1 | 0 |

All other properties would retain the original computed BL. This policy is
applied only after the original nonnull property, admission and matched-ID
guards. It does not manufacture a matching entry for an empty/invalid queue.
Every retained seasonal vector declared an 8027 entry, so the recovered 8027
ID supplies that match when the original queue getter returns it.

The allowlist is the exact property/owner joins in these findings:

- [Gridania and Ul'dah public Halloween](city-halloween-owner-joins.md).
- [Limsa public Halloween](limsa-seasonal-weather-extension.md#halloween-selector-and-resident-group).
- [Three-city interior Halloween](city-interior-halloween-owner-joins.md).
- [Five retained interior winter pairs](city-winter-owner-joins.md).

Names alone are insufficient to register a live pointer. On the retained
TimeLine base table, slot-2 cell `0x01062084` selected `0x008D5430`, which
adjusted its receiver and tailed to `0x00AA89B0`. For a nonnull attached
record, that helper returned the record's `+8` name pointer at
`0x00AA89BC`. The loader/owner and exact record identity must accompany that
name when registering an allowlisted property. The
[selected city record-delivery contract](city-timeline-record-delivery.md#creator-selection-and-attached-record-handoff)
binds the relocated TimeLine node to its base's attached-record cell through
`0x00A8B350`; its loaded-payload base and DAT-relative offsets remain distinct.
Destruction or reload must
invalidate the registered pointer. The
[record-view contract](timeline-weather-record-view.md#construction-and-pointer-attachment)
owns the conditional property-to-instance-to-base lineage.

The [ShowHideClip receiver contract](showhide-clip-dispatch.md#fallback-operations-and-indexed-actor-branches)
owns the separate operation receiver, actor binding and flag setter. Selecting
a condition bit does not itself prove that those receivers were constructed
or that a seasonal clip rendered. The winter allowlist covers the retained
interior family; it does not create additional historical placements.

## Reload and implementation boundary

The extension would retain a logical seasonal mode, not pointers to old
properties, children or weather work sets. It must rebuild its allowlist from
newly admitted owners and resubmit the selected weather key after readiness.
The native state-3 direct-key handler retained only a reverse-resolved ID.
Because these keys all reverse to 8027, that pending ID cannot preserve
which seasonal atmosphere key was requested.

The [reset and ordering contract](seasonal-decoration-update-order.md)
owns request replay and the traced native reload boundaries. Retained request
bytes are not a substitute for a new selected key or fresh pointer admission.
This profile avoids the short scheduler-alias restriction, but still depends
on native owner/context construction, clip execution and weather admission.

The concrete callbacks offer observation points, not a complete replay gate.
Scene slot 32's `0x004EAB20 -> 0x0062AA70 -> 0x00626DF0` path constructed
a fresh owner at A+`0x17C`. Construction and ordinary update reached the
binding matcher through `0x00642944/0x0064A5C6`; successful binding cleared
request-completion bytes at `0x0080692D` and returned its B+`0x78` test.
Those facts establish owner construction and instance binding only.
The unjoined scene-phase dispatch and actor-ready gate must be resolved
before treating either return as a safe raw-key replay point. Logical mode
can survive reload; pointer registrations cannot be carried through the
separate owner and map-object destructor paths.

No extension, hook, file substitution or runtime test is supplied by this
finding. A complete replacement needs the exact receiver and reload joins,
followed by an attributed Halloween/Christmas witness with file redirection
disabled, all selected keys retained, and ordinary 8032 behavior checked.
