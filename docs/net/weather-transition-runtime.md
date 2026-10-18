# Weather transition runtime

This note documents the packet-driven weather transition state in the pinned
FFXIV 1.23b executable. The image has base `0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Packet fields and publication

The opcode tables at `0x004dc71a..0x004dc749` route game opcode `0x000d` to
the weather packet path at `0x004dcbf7`. Its case at
`0x0059cf79..0x0059cfb8` reads:

- weather ID as an unsigned 16-bit integer at packet `+0x10`;
- transition duration as an unsigned byte at packet `+0x12`.

The byte is zero-extended and staged at receiver `+0xb4`. Values 0 through
255 are therefore valid; 180 is not a signed-byte overflow. A wider server
value is truncated on the wire: 256 is observed as 0 and 300 as 44.

At `0x0059ec7d..0x0059ec9f`, the client converts the staged duration to a
whole-seconds counter with a zero fractional part. `0x0059e4d0` publishes the
duration before publishing the weather ID. The duration travels through world
message 114 (`0x0059e1f0` sender and `0x0062d004` receiver).

## Elapsed and target counters

The duration receiver at `0x007e0370` stores the new target counter at
`WeatherManager+0x148`. It clamps the elapsed counter at `+0x138` down to the
new target when elapsed is greater, but it does not reset elapsed to zero.

`0x007e2330` computes the weather fraction as:

```text
(elapsedWhole * 30 + elapsedFraction)
-------------------------------------
(targetWhole * 30 + targetFraction)
```

The result is clamped to `[0, 1]`; a zero whole-second target completes
immediately. `0x007e91d0` and `0x007ea090` convert engine delta time to the
30-frame counter and accumulate elapsed time. `0x00a20380` supplies the
300,000-tick native timebase.

## Resource queue behavior

The weather resource queue contains 12-byte entries. `0x007e87f0` searches
the complete queue by resource ID. If the requested resource is already
present, it returns without inserting or reordering an entry. Appending a new
queue resource through `0x007e80f0` resets the elapsed counter at `0x007e81fa`.
`0x007e78d0` blends the oldest and newest DrawEnv resources and retires the
oldest after the weather fraction reaches 1.

This ordering means a duplicate weather packet can change the duration of an
active same-resource blend without restarting its elapsed clock. For example,
30 seconds elapsed against a 180-second target becomes complete if a duplicate
sets the target to 20 seconds. Conversely, 10 seconds elapsed against a
20-second target moves from 0.5 to about 0.056 if the target becomes 180.
These values are deterministic applications of the native formula, not a
claim about a particular captured crossing.

## Layout providers and environment receivers

Retained Ghidra listings and original PE bytes were checked with pefile
`2024.8.26` and Capstone `5.0.7` in x86-32 mode. The addresses below are
VAs in the executable identified above.

Let O be the weather object constructed by `0x007E8AE0`, with its supplied
parent retained at O+`0x48`. On the inspected RaptureActorContainer
construction path, the container's retained world pointer reaches that
parent's `+0x114`. Helper `0x00623C70` queries the world's slot 8, then
the returned controller's slot 1 and the returned layout-system interface's
slot 3. The concrete RaptureWorld slot 8, `0x00410780`, reads the pointer
stored at world `+0x18`. The controller route through
`0x00A5E8D0/0x00A886E0` returns the layout-system `+4` interface, whose
slot 3, `0x00A88820`, reads the layout-world interface pointer stored at
`0x0136578C`. `0x00623C70` subtracts four from its nonnull result to
obtain the complete RaptureLayoutWorld object.

That object's primary table `0x00FBFB50` has slot 4 `0x00648570` and
slot 3 `0x00648600`, used by `0x007E87F0/0x007E88B0` respectively.
Their factory path reaches `0x006520B0`: its mode-zero branch constructs
MapLayoutWeather through `0x0083C970`; mode one constructs
MapLayoutWeatherSky through `0x0083C390`. The helpers recover the complete
object from the returned `+4` interface before passing it to
`0x007E80F0`. These are conditional construction paths. The finite direct
relative-transfer and literal-pointer searches found no incoming reference
to `0x007E88B0`; that result does not exclude other indirect forms.

The resulting pointer has two destinations and a shared name-source path:

| Native path | Pointer and use |
| --- | --- |
| `0x007E80F0`, argument 3 equals 1 | Stores its object argument at O+`0x7C` for replacement lookup. |
| `0x007E80F0`, other mode branch | Appends a 12-byte O+`0x6C` queue record with the object at record `+8`. |
| `0x007E78D0 -> 0x007E6690` | The blend branch passes the last/newest record's `+8` as argument 1 and the first/oldest record's `+8` as argument 2. On the continuing branch, `0x0083A750` supplies the `+0x3DC` trees for the pair after readiness checks and substitution. |
| `0x007E90F0 -> 0x007E7D90` | Uses the O+`0x68` object's `+0x3DC` tree as the name source and O+`0x7C` as the replacement object. |
| `0x007E9400` fallback | Clears O+`0x7C`. Its nonempty-queue branch recovers O+`0x68` from the first record's `+8` and enters `0x007E9160`, whose name rebuild requires nonnull O+`0x68` and a true slot-18 result. |

The mode-one branch returns before queue insertion and elapsed-counter
reset. On the blend branch, `0x007E78D0` retires the oldest record before
selecting the next pair when the fraction reaches one with multiple entries.
`0x007E6690` tests the resources' primary slot-18 results. An initially
false result for argument 2, the oldest resource, exits without tree work.
A false result for argument 1 enters a fallback that uses the oldest resource
for both inputs, sets the fraction to zero and queries readiness again.
These are resource guards and local update order, rather than selected
actor/model readiness.

### Mode-one ID and wrapper ordering

The retained mode-one callers supply different ID inputs.
`0x007E8DF0` saves its weather selector at O+`0x64` and resolves it
through the parent's `+0x154` manager using `0x0079B380`. That lookup
matches an entry's `+0xB0`. `0x0079B220` then returns the first child
`+0x60` whose child `+0x5C` equals 1. A nonzero result reaches
`0x007E87F0` with mode one at `0x007E8E7B`.
The other caller, `0x006242B0`, requires actor `+0x1B8`, saves its
supplied argument at actor `+0x1A8`, and forwards it with mode one at
`0x006242E8`. These paths do not supply a fixed selected-layout ID.

For the retained root 206, `0x0079B220` returns zero: its only child
has type `0x3000`. `0x007E8DF0` therefore skips the mode-one call on
that table result. The selected layout's concrete key/name and ordinary
manager branch are in
[Selected floor key and name](../resource/map-layout-selector.md#selected-floor-key-and-name).

The alternate `0x006242B0` input remains separate. Its inspected direct
caller is `0x00629492` in dispatcher `0x00629410`'s operation `0x0E`
branch, which forwards the dispatcher's second argument. At `0x0062AB00`,
the queued-command path supplies the pair stored at entry `+0/+4` as
operation and value. The pair's value is not derived from the first
type-1 child by these bodies.

One checked producer is message handler `0x0062CDA0`: operation `0x0E`
reaches `0x0062CDDE`, reads the dword pointed to by its second argument
at `0x0062CDE2`, and passes the original operation and that value to
`0x0062C4D0`. That method appends the pair to actor `+0x33C`.
The upstream producer of the message's pointed-to dword remains unjoined.

On the constructed Sky path, setup `0x00642F20` saves that ID at
complete object `+0x158`, mode at `+0x150`, and state 1 at `+0x140`.
Its registration through `0x0062C550` precedes the provider's return
and `0x007E80F0`'s O+`0x7C` store in the inspected direct flow.
The actor's update `0x0062B220`, under its `+0x1B8` guard, calls the
owner at actor `+0x158` through `0x007E91D0`. That body dispatches the
complete pointer at O+`0x7C` through primary slot 6, Sky `0x0083B500`,
which calls the state dispatcher `0x00648950`.

This connects the stored replacement object to the
[request, completion and loaded-wrapper path](../resource/map-layout-request-boundary.md#registered-states-and-typed-view).
O+`0x7C` retention does not establish file completion, an accepted wrapper
or readiness. The actual ID/name, acquired typed node and active source/Sky
records remain inputs; the retained selected DAT alone does not identify
this request.

The concrete MapLayoutWeatherSky `+4` table `0x0103CC3C` has slot 24
`0x00A8AC40`. It forwards name lookup to its manager's slot 16,
`0x00A898B0`, through interface `+0x1C`, complete object `+0x20`.
The complete finder hashes the supplied name, compares candidates' slot-2
names and returns the first matching candidate pointer unchanged at
`0x00A89995`. This finder does not test kind `0x0D`.
With nonnull O+`0x7C`, `0x007E7DFE` calls this `+4` interface slot 24.
The subsequent guarded minus-four/plus-four adjustments preserve a
successful interface result.
`0x007E7E29` saves it in the working tree's node `+0x64`.

Name admission is separate from the kind map. Sky interface slot 12,
`0x0064F430`, reaches `0x00A8B350` after resource type-chain checks and
a zero result from resource-interface slot 23 on the record.
That helper resolves a matching creator and its kind, then calls manager
slot 1 to construct an object. This creation path, `0x00640270`, requires
the kind policy `0x00624D10` to accept the kind before construction.
Its decision depends on singleton `0x0133DEF4`, kind-collection and
resource/actor state inputs. A successfully created object receives the
record through slot 7 before forwarding the record/object pair through
resource-interface slot 17, `0x00A8ABE0`, to manager slot 8,
`0x00A8A190`. The latter inserts the hash and supplied pointer into the
name buckets consumed by `0x00A898B0`. The eligible record, matched creator,
kind-policy result, admitted pointer and matching name remain inputs.
The name registration and lookup bodies do not establish a universal
kind-0D or RTTI whitelist.

### Record and lookup-name inputs

Python `struct` and `hashlib` inspection of the
[selected layout](../resource/floor-import-contract.md#selected-inputs-and-method)
checked these definition records. Offsets in this table are file offsets;
their stored pointers are relative to the LYB payload at file `0x190`.

| Record | Definition / descriptor | Class ID | Object name / stored hash |
| --- | --- | --- | --- |
| Selected floor BGChip | `0x1E60 / 0x16DC` | `0x0B0B` | `c0_00_plt00_h / 0x0428` |
| ScreenEnv | `0x219C / 0x1790` | `0x0F87` | `scev_0000 / 0x02D0` |

Both descriptor chains include `BaseObjects/BaseObject`. Each record's
stored hash equals the signed-byte sum of its name. These are distinct
object names. The BGChip's
[primary resource key](../resource/bgchip-model-binding.md#creator-and-attached-key)
at record `+0x20` is a separate input from the object name at `+8`.
These retained definitions do not identify the resource currently loaded
by Sky or the name supplied by an active source tree.

The constructed factory's initial registry resolves these class IDs
uniquely. `0x00A90FD0` supplies default registrations; `0x00643800`
removes and replaces the application categories. Registration
`0x00A90F50` keys creators by slot 5's kind. Every surviving creator's
complete slot-6 ID comparison was checked against the original PE.
No other initial matcher accepts either selected class ID.

| Class ID | Registered creator and kind | Created interface |
| --- | --- | --- |
| `0x0F87` | Table `0x0103C498`, matcher `0x00AA6760`, kind `0x0D` | `0x008394F0 -> 0x008D49F0` returns complete object `+4`, with table `0x01061BAC`. |
| `0x0B0B` | Table `0x0103C2B4`, matcher `0x00AA5180`, kind 1 | `0x00837FB0 -> 0x008D3E00` returns complete object `+4`, with table `0x0106126C`. |

This result requires successful construction/registration and unchanged
factory/core aliases, registry and ID cells. Creator matching by class ID
and the later creator lookup by kind are separate operations.
For both created interfaces, slot 7 attaches the record before name
registration. Slots 2/3 obtain its name from record `+8` and hash from
record `+0`. `0x00A8B350` registers that pointer before calling its later
slot-36 and slot-9 initialization methods. Registration alone does not
establish readiness.

`0x0083C110` requests kind `0x0D`, then obtains each admitted object's
slot-2 name for the `+0x3DC` tree keys. The ordinary kind-1 BGChip does
not enter that collection through these getters. If the checked ScreenEnv
record supplies a source key, its name is `scev_0000`; the checked floor
BGChip's different name cannot satisfy that comparison. A matching
ordinary ScreenEnv object has the float-getter interface above.
Other same-name objects, replaced creators and changed records remain
separate inputs because the replacement finder itself has no kind test.

### Creation policy inputs

Let S be the constructed MapLayoutActor loaded from global `0x0133DEF4`, R the
complete resource and K the creator's kind. The complete body of
`0x00624D10` selects these conditions in order:

| Condition | Creation policy result |
| --- | --- |
| S+`0x1F0` equals 3 | Calls `0x00801A70` on S+`0x350`. It rejects K when a collection entry's `+0x100` equals K, and accepts it when no entry matches. |
| Other scene state, K differs from `0x0F` | Accepts. |
| Other scene state, K equals `0x0F`, R is null | Accepts. |
| Other scene state, K equals `0x0F`, R+`0x150` differs from both 0 and 1 | Accepts. |
| Remaining kind-`0x0F` case | Accepts exactly when S+`0x335` is nonzero. |

The resource mode and creator kind are separate inputs. A kind of `0x0D`
therefore uses the collection predicate in scene state 3 and passes this
policy in other scene states. A passing policy still precedes the manager's
own creation guard and the actual creator call.

Constructor `0x0062D520` initializes S+`0x1F0/+0x1F4` to 1 and
S+`0x335` to 1. It publishes S in `0x0133DEF4` before constructing
the control object retained at S+`0x350`; teardown `0x0062DA70` clears
the control pointer and singleton. Publication does not establish safe
use before construction completes.
`0x006274A0` writes its incoming mode to S+`0x1F0`, with saved-mode
handling at `+0x1F4`; `0x0062D1C0` supplies argument 3 to that writer.
`0x0062B660` restores the saved mode. The inspected state-2 writers
`0x00624240/0x006242B0/0x00626E80` require the old mode to differ
from 3, alongside their other local guards. Incoming request selectors,
controller receivers and request IDs remain inputs.

The control object's denied-kind vector has begin/end at `+0x28/+0x2C`.
`0x00624D70` consumes nine mask bits using table `0x012B8170`:
bit 8 selects kind `0x0D`, while bit 0 selects kind 1. A set bit removes
the corresponding denial through `0x00802F30`; a clear bit adds a missing
kind through `0x00805330`. Its inspected calls in `0x00815F00` take a
zero-extended word from record `+0x22`. The control object's tuple and
name collections, edited by `0x00624CD0/0x00624DD0`, are separate.
These kind-admission bits do not identify a selected model mask call.

Setter `0x0062C710` writes the incoming low byte to actor `+0x335`.
The inspected call at `0x00613FC4`, selector `0x15` of `0x00613B10`,
uses the first byte of argument 2 and loads its actor through
`[[caller+0x118]+0x78]`. That receiver must alias S for this call to
change S's policy byte. Resource setup methods `0x00642F20/0x00642F60`
write their argument-2 mode to complete resource `+0x150`; the ordinary
mode-one Sky provider supplies 1 there, independently of S+`0x1F0`.

The manager guard is interface `+0x11C`, complete object `+0x120`,
bit 0. Constructor helper `0x00A8A370` initializes it clear.
Bulk teardown `0x00A89C60` sets it before traversing managed objects
and clears it on normal completion. Interface slot 3, `0x00A89DF0`,
adjusts the receiver by minus four before reaching that helper.
These inspected writers explain conditional creation admission without
supplying current policy values, selected actor identity or setter order.

### Environment slot-85 contract

Raw complete-object locators
identify environment tables `0x01061BAC/0x010A6E8C` as interfaces at
complete-object offset four, with these RTTI names respectively:

- `.?AVRaptureScreenEnvBaseObject@Layout@Map@Actor@Scene@Application@@`
- `.?AVScreenEnvBaseObject@Layout@Stella@Lay@Engine@CDev@SQEX@@`

Both tables' slot 85 is `0x00AB0010`. Its complete body calls the
receiver's slot 8, loads a pointer from the result's `+0x1C` and returns
the float at that pointer's `+0x18` in x87 ST0.
`0x007E2FED/0x007E2FFD` call slot 85 on their two incoming environment
interfaces and consume float results. `0x00A73DF4` makes the same slot
call and stores its float result at destination `+0x148`. The inspected
call windows do not push a mask argument.

These concrete getter contracts are distinct from the selected
[BgPlateActor mask forwarding](../resource/floor-model-render-path.md#mask-updates-and-caller-boundary).
The shared slot number does not join these interfaces to that actor or
model setter. Other receivers, aliases and callback effects remain
separate static questions.

## Independent spatial fade

`0x007e64c3..0x007e6536` advances a separate spatial DrawEnv fraction at
`WeatherManager+0x8c` by `deltaFrames * 0.0166666675`. It completes in 60
native frames, approximately two seconds, and does not read the packet
duration. Local lighting or fog changes driven by this fraction therefore
cannot be lengthened by increasing opcode `0x000d`'s transition byte.

The `+0x8c` value is not the packet-driven weather fraction. Any diagnostic
which samples it alone cannot measure a 20- or 180-second packet transition.

## Zone-name and micro-area boundary

The same executable constructs an `AreaBase` string member at
`+0x64` (`0x006F3252..0x006F3255`). The `_getZoneName` binding at
`0x006F9700..0x006F9715` passes that member to the string-return helper;
registration at `0x00749BC7` loads the binding address. This getter does
not compute a name from coordinates. The weather packet reader above also
receives no coordinate or micro-area field. These observations do not
recover historical spatial name/weather polygons or prove that no other
system could select an area name; a nearest-anchor partition derived from
client place labels and contributor spawn coordinates is a reconstruction,
not a retail boundary table.

## Evidence boundary

The binary proves field widths, counter math, queue behavior, and the separate
spatial fraction. It does not prove which visible components a particular
layout binds to either transition, or reconstruct every rapid A-to-B-to-A
visual outcome. No live-client observation is required for the native claims,
and those historical visual outcomes remain unresolved where no capture
exists.
