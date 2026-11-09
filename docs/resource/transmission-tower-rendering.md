# Transmission Tower rendering

This note separates the retail Castrum Novum Transmission Tower layout from
weather and reconstruction assets that can replace its initial render state.
Native addresses are VAs in the pinned FFXIV 1.23b executable,
`ffxivgame.exe`, PE32 i386, 15,996,808 bytes, with image base
`0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Content and layout identity

Client tables join content 13, Castrum Novum Transmission Tower, to zones 251
and 264, zone parameter 5014, and layout 511. Region resource
`c04b0d998aea4c1b13ed322292a5aa5af45485c698da2315171c3c024bcb9a74`
maps that layout to `lak_l0_dun01` at DAT key `0x03e7000b`.

The 22,512-byte layout at `data/03/E7/00/0B.DAT` has SHA-256
`b7598ff9aedc1af4cda3cefb3372cc83e49e51445b2eb04ba77905d250827088`.
It contains 67 placed instances across four divide-map folders, 12 nested
unit-tree members, two draw-environment objects named `dwev_20` and `dwev_00`,
five authored lights, one screen-environment graph, and two compiled
timelines.

Both DrawEnv records set feature-mask bit 2 and its conditional flag. Native
getters at `0x00a81710`, `0x00a81730`, and `0x00a81740` return the enabled
conditional value, an otherwise unlabeled scalar of 500.0, and mode 0. The
binary does not provide safe retail names for the scalar or mode, so those
fields remain semantically unresolved.

## Authored lights and screen environment

Native light property tables identify serialized byte `+0x44` as LightType.
Named comparison resources encode directional, point, and ambient lights as
0, 1, and 3. All five Tower lights encode 1 and are therefore point lights.

| Light | RGB | Intensity | Radius value |
| --- | --- | ---: | ---: |
| `lght_0001` | `#a8fdf0` | 1.0 | 40.0 |
| `lght_0002` | `#fc1603` | 1.0 | 6.0 |
| `lght_0003` | `#389afc` | 3.0 | 20.0 |
| `lght_0004` | `#fda964` | 1.0 | 21.2 |
| `lght_0005` | `#fff9ca` | 1.5 | 11.0 |

Each light has zero decay and a cone-angle value of pi/6. The same radius
candidate is serialized twice; only one position has a recovered native
label, so the second is not independently named here.

The `scev_0001` graph links `glare_0001`, `shadow_0001`,
`ColorCorrection_0001`, `Blur_0001`, and `VolumetricLight_0001`. Their
parameter banks are present, but all five native enabled bits are false in the
retail Tower layout.

The complete timeline inventory is only `time_bg_gate_open` and
`time_bg_gate_close`; it contains no `LayRapturePointLightClip`. A comparison
dungeon layout does contain explicit point-light timelines, so the absence is
structural rather than a decoder blind spot. The Tower's five point lights are
authored layout state, not a light animation bank controlled by its shutter
scheduler.

## Region and deferred placement

Region resource `0x03C00000` at `data/03/C0/00/00.DAT` has its region-105
root at file `0x1810`. Four of its 15 children have types other than 0
or 1. The [normal RegionInfo submission](map-layout-selector.md#normal-construction-and-name-resolution)
passes all four to LayoutWorld.

| Child key | Type | Resource | File offset |
| ---: | --- | --- | --- |
| 581 | `0x1000` | `0x03E70000` | `0x1A50` |
| 501 | `0x2000` | `0x03E70001` | `0x1A80` |
| 511 | `0x2002` | `0x03E7000B` | `0x1AB0` |
| 591 | `0x4000` | `0x03E7000F` | `0x1AE0` |

The type-`0x2002` submission takes the ordinary RaptureLayoutManager
constructor at `0x00651CD0`. The SetMap zone dword follows a separate
route through selector `0x0A` at `0x0059E44F`. Its immediate receiver
stores it at scene `+0x198` at `0x0062CDCA`. The region transition copies
it to scene `+0x194` at `0x0062D204`. The non-mode-3 special-zone
comparisons test 239, 250 and 256. A previous zone in that set can
still set scene `+0x1FF` at `0x0062D221` when the destination is 251 or
264. The normal region submission loop does not read the zone dword.

The shutter Instance occurs in deferred folder `Bk_ibgpl_000001` at
file `0x4930`. Its member slot 1 uses wrapper `0x4A74` and refers to the
Instance at `0x2E10`. Instance byte `+0x14` is `0x05`, with automatic
bit 1 clear. Folder bytes `+0x60..+0x63` are `00 00 02 00`. The zero
`+0x61` becomes priority zero at `0x0064279C`. Clear folder `+0x62`
bit 0 selects deferred registration through `0x0064AF50`.
The folder's `+0x50/+0x54` registration array names `lak0Dungeon01`
and `lak0Dungeon01a`. Its separate area-pair pointer and count at
`+0x58/+0x5C` are zero. The auxiliary geometry fields
`+0x64/+0x68` are also zero.

| Folder bounds | Minimum XYZ | Maximum XYZ |
| --- | --- | --- |
| Serialized local f32 | `(-23.999998092651367, -2.3841856489070778e-07, -23.999998092651367)` | `(23.999998092651367, 0.001999999862164259, 39.0)` |
| Expanded local f32 | `(-24.009998321533203, -0.010000238195061684, -24.009998321533203)` | `(24.009998321533203, 0.012000000104308128, 39.0099983215332)` |

The registration path at `0x006426C0` expands each endpoint by the
binary64 value `0.009999999776482582` at `0x00FBF9D0`, then stores f32.
LaySettings at file `0x1CE0` supplies translation `(0,-100,0)` and
radius 128.0. The ordinary manager's world-to-local point method,
`0x00A8BDF0` from cell `0x00FBFE60`, subtracts that translation at
`0x00A8BE14`. Area selection and streaming call the point conversion
through `0x00644840` and `0x006433D0` respectively.

Scene `0x00629C40` refreshes the focus at `+0x164` before the area
resolver. On the ordinary context path, `0x00624950` obtains a candidate
through context `+0x64`, then tests its virtual `+0x2E4` predicate.
`0x006243A0` reads the selected focus position through virtual `+0x88`.
A constructed world position `(0,-100,22.66200065612793)` maps inside
the shutter folder. The point is a coordinate witness for containment.

If the ready Tower manager owns the current area with ordinary scene
flags, the folder's zero area pairs leave mode 4 at `0x00629C22`.
The manager's mode-4 path calls `0x00650020`. The current-area manager
name admits the primary radius route through `0x006432E0`. With the
snapshot gate admitted and the folder flags eligible,
`0x0064C580 -> 0x0064BF60 -> 0x0064AC60` can request the block at
`0x0064C200`. Its radius test calls `0x00640040`, whose clamped AABB
distance is zero for the witness. The [area and block selection rules](layout-block-streaming.md#area-resolution)
describe sibling comparisons and other modes.

An admitted state-1 block reaches the folder's member walk at
`0x00640870`. On a missing instance, `0x00640913` can create it through
`0x00A8D0B0`. The selected parent descriptor at file `0x3D90` supplies
child key 1 and byte `+4=0x05`. The child admission path forwards that
byte's bit 2 to slot 51. Key 1's BGParts reference resolves the declared
`brt` key `0x72E50063`. Focus history, the actual winning sibling
manager, model completion and visible placement are outside this trace.

The original PE instruction and table reads use the pinned image above.
The resource records follow the declared RegionInfo counts and
[LYB node shapes](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/docs/formats/world-layout.md).

## Declared collision and support witness

The shutter folder's 16 instances contain no Attribute or CollisionBox.
Its six distinct visual base keys resolve to `brt` rows. The two
selected shutter timelines carry a transform and a sound clip, and
neither addresses the two declared Attribute occurrences below.

| Deferred folder | Instance and member | Attribute and PHB |
| --- | --- | --- |
| `Bk_isgrp_000001` at `0x48C0` | Instance `0x2B50`, `sgrp_room` key 7 at `0x2500` | Attribute `0x3FDC`, row `0x1C0`, `0x72E30099` |
| `Bk_isgrp_000011` at `0x49A0` | Instance `0x2DD0`, `sgrp_taihi_room` key 3 at `0x25F0` | Attribute `0x4008`, row `0x300`, `0x72E300C3` |

Both rows use the `bhp` tag. Their installed PHB identities are below.

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| `data/72/E3/00/99.DAT` | 153296 | `9507ef1dc5b3c55f415a9e52690042c62470479c7a37f0dc3f2af45d4c8feee4` |
| `data/72/E3/00/C3.DAT` | 51408 | `db7e9de463a329a838e8271b630fd36d9c3f4393e84d29e438c9265dbd8f6da3` |

The first Instance, its UnitTree and child have identity transforms.
The second differs by Instance Z translation `51.444000244140625`.
LaySettings translates both by `(0,-100,0)`. The first room's hull
extends beyond its owner's streaming AABB.

A vertical line at world XZ `(0.5,22.66200065612793)` crosses two
strict triangle interiors in `0x72E30099`. Both are horizontal with
positive Y winding and raw surface 0. Adding the authored Y translation
gives `-99.80995951592922` for triangle 937 at file `0x16D08` and
`-100.00000023841856` for triangle 994 at `0x16ED0`. A bounded offline
grid/tree walk at the local intersections reaches both references in
grid cell 332, in TBCs 134 and 159 respectively. The
[quantized tree query](phb-branching-tree.md#tree-quantization)
supplies the offline point-box rule. The triangle and transform reads follow the
[PHB format](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/docs/formats/phb-collision.md)
and the declared LYB instances.

Both room folders have `+0x62` bit 0 clear for deferred registration,
priority zero and zero area pairs. The expanded bounds of the first room, shutter and second
room are each within 16 units of the constructed focus on that line at
world Y -100. With a ready Tower manager owning the current area, admitted
mode-4 radius processing can request all three even though the point is
inside only the shutter folder.

The selected room member descriptors at file `0x3C28` and `0x3D84`
each have byte `+4=0x05`, whose bit 2 reaches child slot 51. The
Attribute's attached group value is 1 in both cases. On the admitted
category-`0x0B` path, `0x008D3DB0 -> 0x00AA81D0` sets up the Attribute
and `0x00AA8480` supplies the collision group. On the checked single-result
query path, the shape-flag and group gate at `0x00B0CAF3..0x00B0CB16`
requires mask bit `0x2` to admit these group-1 shapes. Triangle and
other filters apply separately. Query admission, accepted footing and
movement remain open.

## Shutter contract

Layout instance `isgrp_000012` binds packet instance ID 12 to unit tree
`sgrp_shutter`. Its serialized node GID 935 is a separate identifier.
The declared instance at file `0x2E10` references that UnitTree at
`0x2480` through instance `+0x3C`. Its direct child keys are unique.
Child `+0x14/+0x18/+0x1C` supplies the name, definition and key.

| Child key | File offset | Child name | Definition |
| --- | --- | --- | --- |
| 1 | `0x2620` | `l0f0_q1_shtr1_h_001` | BGParts `l0f0_q1_shtr1_h` at `0x4130` |
| 2 | `0x2650` | `open` | TimeLine `time_bg_gate_open` at `0x4800` |
| 3 | `0x2680` | `clos` | TimeLine `time_bg_gate_close` at `0x4828` |

The instance's authored translation is `(0, 0, 22.66200065612793)`.
The UnitTree and key-1 placement have zero translation. All three have
zero rotation and unit scale. These values describe the serialized placements.

### Authored transform and sound clips

TimeLine `+0x14/+0x18` gives the SCB reference and length. Both
`LayTransformClip` records select actor ordinal 0 at record `+3`. Its
`LayUnitMemberActor` record holds key 1 at `+0x20`. Original table cell
`0x0109F6DC` selects getter `0x00A78950` for that field. This joins both
clips to the same direct shutter member.

| Alias | SCB file range | Actor ordinal 0 | Transform record | Extent |
| --- | --- | --- | --- | ---: |
| `open` | `[0x4C50,0x5240)` | `0x4D20` | `0x4E60` | 100 |
| `clos` | `[0x5240,0x57F0)` | `0x5310` | `0x5470` | 25 |

Both transforms start at raw value 0. Original table cell `0x010A14FC`
selects `0x00A0DBC0`, which reads the end at record `+0x10`.
The ends equal their active block spans, 1,000,000 for open and 250,000
for close. Each block also contains one `LaySEClip`. The open sound span
is 10,000 to 300,000. The close span is 0 to 290,000 and exceeds its block
span by 40,000. Each scheduler also declares an empty block.

### Translation curves

Both transform records store mode 0 at `+0x14`, flags `0x0004` at
`+0x18`, curve ordinal 0 at `+0x1A`, and the signed extent at `+0x1C`.
The `@FCURVE` chunks start at file `0x4F10` and `0x5500`. The u16
offsets at `0x4F32` and `0x5522` are zero and select the first
descriptors at `0x4F40` and `0x5530`. The resolver at `0x00A220A0`
uses 16-byte units for these offsets. The MTB bodies at `0x4F60`
and `0x5550` are 518 and 454 bytes respectively.

Each body declares `TransForm` mask `0xE077`. The X and Z translation and
three rotation channels are type-0 zero constants. The three scale
channels are type-0 unit constants. The varying Y channel is a type-2
Hermite curve at file `0x5030` or `0x5620`. Its records contain four f32
lanes, in coordinate, value, incoming and outgoing order. Native
`0x00A2DF10`, `0x00A320B0` and `0x00A31F70` establish that ordering and
evaluation. Open has ten keys from `(0, 0)` to `(100, 4.5)`. Close has
six from `(0, 4.5)` to `(25, 0)`, including the negative key near the end
at coordinate 24. The installed curve supplies the intermediate values
and tangents.

`MoveRatio.tx` at file `0x5128` or `0x56D8` is type 4 with flag zero.
Its two keys range from `(0, 0)` to `(100, 1)` or `(25, 1)` respectively.
Native loader `0x00A2E0C0` and evaluator `0x00A33610` agree with the
serialized f32 value followed by a signed 16-bit coordinate.

### Conditional native application

For retained `LayTransformClip` table `0x010A14DC`, begin resolves the
curve descriptor at `0x00A0ED5A` and caches Y at clip `+0x40`. The
`0x00A0DC50` path takes local elapsed value L and signed duration d,
bounded below by 1. It samples MoveRatio at extent times L divided by d,
clamps the ratio to [0,1], and multiplies by the extent. The ideal sample
coordinate over either authored interval is L divided by 10,000.
Floating-point rounding and actual elapsed values govern delivered samples.

Update `0x00A0E6A0` calls the base update first at `0x00A0E6B5`.
Transform sampling then requires an operation context and clear flag
`0x02`. The selected flags admit X, Y and Z sampling, including Y at
`0x00A0E794`. Mode 0 stores the sampled vector at clip `+0x88/+0x8C/+0x90`.
An optional context `+0x48` hook at `0x00A0EC44` receives translation
and rotation by pointer. It precedes the context `+0x34` translation
call at `0x00A0EC5C`. The submitted translation is the unmodified
sample when the hook is absent or preserves that vector. The sampled
rotation and scale branches are skipped. On normal return, the update
still submits rotation through context `+0x3C` at `0x00A0EC76`. That
argument is the retained vector when the hook is absent or preserves it.
Clear flag `0x40` skips the scale dispatch.

The shared [clip delta path](ferry-door-schedulers.md#conditional-ordinary-clip-delta)
governs conditional update steps. These clips do not establish a Tower
wall-clock duration, first or last visible sample, context side effects,
simultaneous request arbitration, collision or audible output.

The offsets were recovered from a declaration-driven LYB and SCB walk
with the [world layout format](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/docs/formats/world-layout.md)
and the [SCB layout](cut-scheduler-clip-layout.md). A bounded Python
struct walk resolves the MTB descriptors. LLVM i386 disassembly and
original PE table reads establish the native paths.

### Shutter member translation

On admitted placement, `0x007FD830` registers child key 1 in the
UnitInfo at `0x007FDB5D` and assigns that UnitInfo to the TimeLine
child at `0x007FDA61`. TimeLine callback `0x00A84F70` obtains that
owner and resolves the constructed actor key through UnitInfo slot 37.
A nonnull member reaches `0x00A78960` at `0x00A8506E`, which stores it
at the LayUnitMemberActor's `+0x64` and registers a release listener.
The transform update indexes the retained scheduler actor collection
through `0x00A1FFA0`. This path requires the same scheduler, actor and
registered member through binding and update.

The class-key operation provider is retained at scheduler `+0x84`.
When its LayTransformClip node lookup returns zero, the fallback
holder table `0x00FBEAAC` supplies its listener through slot 9 at
`0x00B36630`. That listener is the adjusted LayCutCharacterListener
interface with table `0x0109E774`. The update calls the operation
object's slot 9 at `0x00A0E6C2`. A nonzero class-node payload takes a
different provider path with unresolved receiver and slots.

In the selected actor records at file `0x4D20` and `0x5310`,
`+0x14=2` leaves bit 0 clear. The retained actor's slot 8 therefore
admits the optional hook through `0x00A1C050`. On the fallback listener,
`+0x48` selects the return-only `0x00A729F0`. Listener `+0x34` selects
`0x00A72810`, which copies the sampled vector and dispatches actor
`+0x38`. Its retained target at `+0x64` receives the translation
through instance slot `+0x5C`, `0x00A9A7F0`.

That setter can allocate a translation controller when the authored
local XYZ differs from the input. A retained valid controller stores
three f32 lanes at `0x00A8BC29..0x00A8BC2D`. The write replaces the
member's local translation. The effective translation reader reaches
the same entry through `0x00A9AC30 -> 0x00A8BBC0`. Equality with the
authored position can exit early when no controller exists.

On the admitted BGParts property path, `0x00A98020` obtains the cached
parent matrix and `0x00A97DB0` obtains the current local SRT.
It composes them at `0x00A97DEB` and submits the result to model slot 5
at `0x00A97DFD`. A retained LayDefaultModelObject reaches
`0x00625B20`. The ordinary branch with model index -1 and descriptor
bit `0x20` clear can write the DrawableObject matrix through
`0x00A61620 -> 0x00C39B10 -> 0x00BB7550`, subject to its model,
dirty and transform guards. A valid indexed branch instead calls
`0x00C398A0 -> 0x00C3C7E0` for its 0x30-byte record.

Actor teardown at `0x00A788D0` unregisters and clears the retained
target. On member release, a matching listener call through
`0x00A789A0` also clears it. Slot `+0x38` reads the retained pointer
on update. Actual fallback selection, model setup, drawable branch
and visible delivery remain open.

### Start template and terminal sample

Each selected SCB has one `@CCNT` BlockInstance. The records at file
`0x4EC0` and `0x54B0` store template index 1 at `+0` and connection count
zero at `+1`. With scheduler `+0x88=2` and `+0x8C=1`, the start consumer
selects BlockInstance ordinal 0 at `0x00A1D261`. It reads the template
index through `0x00A27BD0` and populates template 1 through `0x00A1BA90`.
Template 1 is the active block tabulated above. Template 0 is empty and
outside this admitted start route. On the ordinary mode-zero completion
branch, the zero connection count yields next-block result -1 through
`0x00A27C50`. With that result retained after any listener, the scheduler
ends in state 4.

The newly begun and running-list update calls at `0x00A1D7D2` and
`0x00A1E1E7` precede the signed duration comparisons at `0x00A1D7F3`
and `0x00A1E208`. Transform slot 2 at `0x00A0E6A0` processes elapsed
first, then samples and dispatches through the conditional context above.
The first update can preserve the elapsed value. With the selected curves
retained, reaching or crossing the duration takes MoveRatio's last-key
return of 1. The fixed curve's last-key branch `0x00A32100` returns the
authored Y endpoint, 4.5 for open and 0 for close. An absent or preserving
hook and a valid context can deliver that sample on the threshold-triggering
update. The ordinary end path for state 2 and phase 0 calls end and
finalize at `0x00A0DAD0` and `0x00A0DAE0`, then unlinks the running clip.
This cleanup has no second direct sample call. Abort, replacement and
other cleanup paths have separate effects.

### Repeated manual requests

The ordinary `0x00D9` request passes the alias and phase zero through
`0x00585320`. The selected route sends selector `0x28`, action
`0x7F`, phase zero and second argument -1 through `0x0059D590`.
Publication requires the retained actor binding and precedes the alias
lookup. Queue helper `0x00805080` removes the first retained exact-name
request through `0x00803130` before appending the new command. Immediate
dispatch at `0x00806320` requires scene state other than 3, exact alias
lookup, a kind-7 member and its manipulator's slot-11 admission. An
unadmitted request can remain pending. The ordinary pending route can
add elapsed time to the phase.

On admitted action `0x7F`, `0x00801270` derives the member's group.
The TimeLine record reader at `0x007FC93D` reads `+0x25`. Both selected
records at file `0x4800` and `0x4828` store zero. The zero value enters
the special-name fallback, where both aliases fail the `set?` check
and return zero. The call at `0x0080250B` walks matching kind-7 group
members through `0x00802210` before the chosen member's start slot at
`0x0080256B`. Matching members with retained manipulators receive slot 4.
This includes both Tower aliases with the expected member tables.
Reaching scheduler reset at `0x00A1F0C0` additionally requires the
retained wrapper, its readiness check and a scheduler. Reset clears
running and held clips without a direct transform sample. The chosen
start slot then requests the selected scheduler with the admitted phase.

After admitted dispatch returns, `0x00803210` removes retained
different-name requests whose aliases resolve to the same group.
Both command-vector removal paths can remove completed records.
A pending request reaches the reset only on admitted dispatch.
Actual operation-context attachment to key 1, timing and visible pose
across two requests remain open.

## Weather and material overrides

Retail Mor Dhona resource 8032, `wtr_xmas`, is labeled Dalamud Thunder and
has DAT SHA-256
`4f462d9ddaf1917b90d519973c42d3ccbd5358930eb5dbfb92b00f09419800bc`.
It adds six thunder dependencies over resource 8030 while retaining the same
FCurve property structure and values. Recovered client Lua contains no static
content-13 to weather-8032 assignment. Resource locality and the thunder
payload make it a comparison candidate, not proof that the duty selected it.

The pinned executable's WeatherManager registration at `0x007e7480`
compares names against `dwev_0`, `dwev_1`, and `dwev_2` family substrings
(literals at `0x00fef4c0`-`0x00fef4e8`). Its caller at
`0x007dce65`-`0x007dce8c` detects a changed spatial-object pointer, obtains
that object's name through vtable offset `0xa4`, and passes the name into
registration. This is a name-family classifier and a selected-object path,
not a static choice of the Tower's `dwev_20` or `dwev_00`. The selected
spatial object and active exact suffix remain unobserved historically.

The native `RaptureMaterial2Clip` implementation gives an independent ABI for
weather material overrides. Initialization at `0x00835250` resolves an
`Attr` FCurve and four channels. Application at `0x00835470` appends `R`,
`G`, `B`, and `A` to its stored name and sends each sampled value to
every material entry in the active container. `Attr` is therefore a four-
channel RGBA material-parameter broadcast, not a Tower point-light control.
The [Lay subclass finding](uldah-halloween-material.md#selected-record-and-inherited-consumption)
locates the stored-name input path and distinguishes the channel-selector
names from those output suffixes; its selected serialized values remain unjoined.

A non-retail weather payload previously compared during reconstruction,
resource 8076 with SHA-256
`6a6c23836b0d6c5955eb0078bf7b23ca3249dde20857249f151c393886c36900`,
inherits all 2,565 FCurve records from its restored Starlight source. Its 119
changed bytes only zero five dependency entries. Its `time_dwev_00` lane adds
`diffuseColor_00` with `Attr=(0,0,0,0)` and `lightMapOcclusi` with
`Attr=(1,1,1,1)` relative to retail clear weather 8001. Those values explain
what the custom payload commands; they do not identify the retail Tower
weather.

## Evidence boundary

The retail artifacts prove layout identity, initial layout components, light
types and parameters, shutter ownership and timing, the DrawEnv name-family
classifier, and the Material2 ABI.
They do not prove the duty's historical weather ID, exact active DrawEnv lane,
server weather bootstrap, or the final enabled state after a weather blend.
WeatherManager behavior is documented separately in
[Weather transition runtime](../net/weather-transition-runtime.md). A custom
payload or a forced weather result must not be promoted into the original
retail assignment.
