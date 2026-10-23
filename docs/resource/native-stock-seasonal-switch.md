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

The extension would retain the logical profile when the ordered scene
weather operation was consumed, translating input 8033 to native ID 8027
before ordinary ID normalization. The stock selector-`0x0C` branch in
`0x00629410` could then retain that native ID in state 3. Input 8033 is
an ID, not a resource key for selector `0x0D`'s reverse lookup.

The proposed key selection would override only these qualified weather
lookup returns for native ID 8027 and a matching profile/city context:

| Original `0x0079B050` callsite | Consumer of the chosen key |
| --- | --- |
| `0x006241F2` | MapLayoutActor current-key field A+`0x1A4`. |
| `0x007E8DD4` | Weather-owner queue path with an existing region owner. |
| `0x007E8E58` | Weather-owner queue path without a region owner. |

All three must select the same profile key. Other callers and IDs retain
stock results. This uses the client's original ID admission and deferred
replay, rather than adding an asynchronous post-bind resource request.
The city must come from the admitted request's region or the matching
`0x0079B050` RegionInfo receiver. Pending-region creation called
`0x00624180(ID,incomingRegion)` at `0x00626E1E` before storing that
region at A+`0x190` at `0x00626E25`; reading only the old retained
A+`0x190` at that point could select the previous city's key.
The [pending-ID contract](../net/weather-transition-runtime.md#pending-weather-id-through-a-region-change)
owns both pending-region branches and the exact local ordering.

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
or that a seasonal clip rendered. The
[display contract](bgparts-display-flag.md) binds the constructed
notification targets, later flag propagation and admitted BGParts model
transform update. The winter allowlist covers the retained interior
family; it does not create additional historical placements.

## Reload and implementation boundary

The extension would retain a logical seasonal mode, not pointers to old
properties, children or weather work sets. It must rebuild its allowlist from
newly admitted owners. The native state-3 direct-key handler retained only
a reverse-resolved ID; that ID alone could not distinguish these keys.
With the explicit logical mode and three qualified lookup returns above,
native pending-ID replay can choose the profile key again without retaining
a raw pointer or scheduling a new post-reload callback.

The [reset and ordering contract](seasonal-decoration-update-order.md)
owns request replay and the traced native reload boundaries. Retained request
bytes are not a substitute for a new selected key or fresh pointer admission.
This profile avoids the short scheduler-alias restriction, but still depends
on native owner/context construction, clip execution and weather admission.

Scene slot 32's `0x004EAB20 -> 0x0062AA70` callback handled both
pending cases. With a pending region, `0x00626DF0` invoked the native
ID setter before allocating the new owner; without one, the callback
invoked it using retained region A+`0x190`. This was a concrete stock
submission order, not evidence that owner construction or B+`0x78`
binding marked complete resource readiness. Reusing the ID path preserves
its original weather-enable, region and manager guards.

A live change between Halloween and Christmas needs an additional explicit
policy because both profiles use native ID 8027. Same-ID exits at
`0x006241A1/0x006241DD`
can skip all three key resolutions. For a mode change admitted in the
original scene-operation context, with valid city/region, weather enabled,
state other than 3 and neither pending region nor pending weather ID
(A+`0x188 == 0` and A+`0x18C == 0`), the proposed policy
is native weather clear `0x006242F0` followed by the original ID request
for 8027. The clear removes the current key/queue state; the new request
then resolves the selected key under normal guards. The
[clear contract](../net/weather-transition-runtime.md#direct-resource-key-requests-and-retained-state)
retains its pending-only exception. The extension must defer this policy
when those conditions fail, preserving its logical request rather than
clearing a stale owner or forcing an early raw-key submission. Activation
of that deferred live change must not replace the mode belonging to an
older pending operation before the new request is admitted. Ordinary-mode
inputs retain their original requested ID, including 8032.

This is a design for using existing native admission, not a new proof of
global thread safety. The outer scene-phase/job synchronization remains
unjoined. A direct raw-key alternative would still need its own proven
execution context; B+`0x78` is insufficient. Logical mode can survive
reload, while property registrations must be invalidated at their separate
owner and map-object destructor boundaries.

No extension, hook, file substitution or runtime test is supplied by this
finding. A complete replacement needs the ordered mode-change/defer policy
implemented and qualified, followed by an attributed Halloween/Christmas
witness with file redirection disabled, all three key resolutions consistent,
decorations re-admitted across reload, and ordinary 8032 behavior checked.
