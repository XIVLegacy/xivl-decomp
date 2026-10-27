# BGChip resource binding and instance setup

The deferred layout block separates resource acquisition, base-object binding
and placed-instance setup. Its later setup phase can supply a BGChip's retained
`brt` resource to a concrete instance property. These instructions establish
a conditional consumer path, not successful resource completion or rendering.
The [BGChip finding](bgchip-model-binding.md) owns the serialized key and initial
child creation. The [activation finding](layout-block-activation.md) owns block
creation and its state-1 callback.

## Binary and method

Addresses are VAs in retail 1.23b `ffxivgame.exe`, image base `0x00400000`,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Subtract the image base for RVAs. Assembly came from
`tools/ghidra_scripts/DumpFunctions.java`. All instruction rows in the cited
function listings were compared with the original PE bytes. The omitted
`0x00A62180` collision loop was decoded directly with 32-bit Capstone.
RTTI and virtual slots came from
`tools/ghidra_scripts/DumpRtti.java`, recorded in `config/ffxivgame.rtti.json`
and `config/ffxivgame.vtable_slots.jsonl`, and were checked against the PE.
No client execution or process observation underlies these claims.

## Block phases and resource binding

`0x00650400` dispatches block full-object `+0x7C` through the table at
`0x006504F8`. Relevant state targets are:

| State | Function | Observed purpose |
|---|---|---|
| 1 | `0x00648770` | Creates placements, collects resource-bearing base objects and selects state 2 or 5 |
| 2 | `0x0064FCC0` | Enumerates each collected object's resource names and types |
| 3 | `0x0064D370` | Processes acquisition and advances when all bind entries return non-null resources |
| 4 | `0x0064D4E0` | Supplies acquired resources to base objects |
| 5 | `0x0064D5E0` | Runs later placed-instance setup |

After the creation callback, `0x006480F0` gets each parent's UnitInfo through
instance slot 61 and calls UnitInfo slot 51 with the block's collection at
full `+0xAC` and a false second argument. Application UnitInfo slot 51,
`0x007FF810`, obtains child base objects through child-instance slot 19.
On the allowed `0x007FBFF0` predicate path, it retains distinct base objects
whose slot 24 reports a nonzero resource count, and recurses into nested
UnitInfos. The block also checks its direct placements. This connects the
created BGChip child to resource enumeration, subject to the collection
predicate and allocation succeeding.

`0x0064FB30` reads base-object slots 24, 25 and 26 as count, name and type,
then calls `0x0064EDC0`. That helper calls `0x0064D790`, which creates or reuses
a `BaseObjectResourceBindInfo`. Constructor `0x0064C840` installs main vtable
`0x00FBFA9C` and stores the base-object interface at full `+0x20`.
Slot 3, `0x005CDCC0`, returns the entry count. Slot 5, `0x00648C00`, returns
the selected entry's resource pointer at entry `+0x28`. `0x00648C40` writes
that field. The acquisition helper `0x00649160` calls it at `0x00649686`.
The state-3 final check, `0x006429F0`, requires every slot-5 result to be
non-null before advancing to state 4. This is a pointer-presence condition.

State 4 calls `0x0064B060` for bind objects whose full byte `+0x18` is clear.
For BGChip category 1, base-object slot 27 is `0x00AA9560`. It tests the
primary resource and every additional resource for non-null pointers.
If that test fails, the helper calls base-object slot 22, `0x00AA93B0`,
with the bind object and a false notification argument. Slot 22 takes the
primary resource from bind slot 5 at index zero and supplies it to model
slot 42. The retained resource therefore reaches the same field consumed
by initial setup.

`0x0064B060` sets bind byte `+0x18` even when slot 22 returns false.
`0x00642AA0` checks those bytes before state 4 advances to 5. Neither that
byte nor state 5 proves that the model accepted a usable resource.

## Later child setup and concrete property

State 5 calls `0x007FF6D0` on the full parent Instance. For a UnitTree
base object, category `0x12`, it obtains UnitInfo through instance slot 61
and calls `0x007FF5A0` on UnitInfo full object. That helper walks created
child instances, gets each child's base object through slot 19, and calls
base-object slot 18 at `0x007FF650` on the fallback setup path. An existing
property can instead take the guarded `0x007FF280` path. It then marks child
full byte `+0x50` bit 0 without checking the slot-18 return value.

For a BGChip, slot 18 reaches `0x008D3EB0` and then `0x00AA97C0` when its
admission bit permits setup. The latter obtains the property through
instance slot 47, requires property category 1 and a non-null model resource,
and calls property slot 46 with `(resource, instance, setup data)`.

The binding helper `0x00A9A730` obtains the property through global layout
factory slot 3 using the base-object category. Application slot 3,
`0x00641690`, dispatches core slot 3, `0x00641220`, which selects the
registered creator through layout-factory slot 23, `0x00A90E90`, and calls
creator slot 3. The category-1 application BGChip creator registered in the
[BGChip finding](bgchip-model-binding.md#lyb-relocation-and-attachment) maps
that slot to `0x00838070`. It constructs the property with `0x007FB120` and
returns full object `+0x4C`. Instance full-object slot 1, `0x00A99EB0`,
stores that returned interface at instance full `+0x18` and gives it the
owning instance interface through property slot 2.

The application property constructor `0x007FB120` installs
`RaptureBGChipInstanceObjectProperty` main vtable `0x00FF242C` at full
`+0x4C`. Its slot 46 is `0x00A97550`. After invoking cleanup slot 47,
it rejects a null resource. Otherwise it calls global `0x01365B04` slot 1
with category zero and stores the returned display object at property
interface `+0x0C`. For a non-null display object, it invokes that object's
slot 1 with the three setup arguments, then calls `0x00A97420` with the
property interface minus four. Property slots 48 and 49, `0x00CB1C80`,
return the stored display object.

`0x00A97550` returns true even if the display factory returns null and does
not test the created object's initializer result.

## Display factory and background actor

Application setup `0x004EA360` first calls `0x00A49480`, which constructs
`LayDefaultDisplayManager` with `0x00A77200` and writes its full `+0x04`
interface to global `0x01365B04`. It then constructs the application
`Application::Scene::Actor::Map::LayDefaultModelObjectFactory` through
`0x004EBAF0`, returning its full `+0x04` interface, vtable `0x00F9327C`,
to display-manager slot 2 at `0x004EA53A`. That slot, `0x00A77120`, uses
factory slot 1 as the registry key and replaces the prior factory for that
key. Application factory slot 1, `0x00AB7340`, returns zero.

Display-manager slot 1, `0x00A76FC0`, finds the requested key through
`0x00A77700` and calls the registered factory's slot 2. For key zero,
the application factory's `0x0062C430` allocates `0x6C` bytes, constructs
`Application::Scene::Actor::Map::LayDefaultModelObject` with `0x0062BE80`
and returns full `+0x04`. That wrapper's main vtable `0x00FBAD6C` maps
slot 1 to `0x00628B80`, consuming the property's three setup arguments.
This registration path establishes the concrete receiver without proving
that application initialization or later allocations succeeded in a run.

The wrapper has an early reuse path through `0x0064DA20`. On its fallback
path, `0x00628CEF` reads the owning instance's base-object category. Category
1 selects actor type `0x15`. The actor factory call at `0x00628D2C` goes
through `0x007C9110` and `0x007A02D0`. The latter selects creator index
`type - 0x0B`. Factory construction `0x007C9850` initializes that vector
through `0x007A0450`, whose index 10 entry is `0x0079FCE0`. That creator
constructs `BgPlateActor` with `0x006235D0`, then `0x006232F0` and
`0x007CEF80`. Its main vtable is `0x00FBA97C`.

`0x007CEF80` passes the model factory returned by `0x007CFAC0` to actor
base construction `0x00A60B80`. This is the singleton at `0x0134DC04`,
initialized by `0x007CFA60` with `RaptureModelObjectFactory` vtable
`0x00FEA500`. Factory slot 1, `0x008C7F70`, constructs the model through
`0x008DD0A0`, which installs `RaptureModelObject` vtable `0x010653A4`.
The actor retains this model at `+0x0C`.

## Primary resource to model candidate array

The wrapper saves its incoming primary resource at stack-local `S+0x30`.
At `0x00628DEB/0x00628DF4`, it pushes a parameter-object pointer and a flag.
The following lookup `0x006259E0` consumes only its own one argument with
`RET 0x04`, leaving those two pushes live. Thus `0x00628DFD` reads
`[ESP+0x38] = [S+0x30]`, the saved resource. Actor slot 15 at `0x00628E0D`
receives four arguments: `(resource, lookup result, flag, parameter object)`.
Its `0x00A602B0` returns with `RET 0x10`; the two earlier pushes are part of
this call, not discarded arguments.

After its resource-interface guard succeeds, `0x00A602B0` invokes retained
model slot 2 at `0x00A603BF` with the resource as argument 1 and zero as
argument 2. Model slot 2, `0x00A630E0`, takes the non-null-resource branch:
at `0x00A63134` it pushes count 1, at `0x00A63136` it takes the address of
its incoming resource argument, and at `0x00A63140` it calls model slot 1.
The other four arguments are forwarded. This bridges the wrapper's
three-argument interface to the six-argument model initializer and supplies
a one-element candidate array containing the primary resource.

The alternate null-resource branch at `0x00A63200` enumerates resources
through model `+0xD0` slots 2 and 3. It does not produce the array for the
non-null primary-resource branch above. The BGChip property's later
`0x00A97420` additional-resource setup likewise is not that array producer.

The [selected floor model trace](floor-model-render-path.md) follows this
initializer into drawable construction, mesh buffers and guarded submission.
The [floor import contract](floor-import-contract.md) separates its offline
write/preservation checks from native rendering qualification.

## Conditional resource-change notification

There is also a listener route. Instance binding `0x00A9A730` registers
the instance's full `+0x08` listener with base-object slot 11. Its main
listener vtable is `0x00FF277C`. When `0x00AA93B0` changes a retained
resource and receives a true notification argument, it calls `0x00AA8AB0`,
which dispatches listener slot 2 with the address of a temporary base-object
pointer. The application instance's slot 2, `0x00A9A450`, dereferences that
argument once and forwards the base-object pointer to property slot 24,
`0x00A971A0`. That function gets the owning instance through property
slot 4 and invokes the supplied base object's slot 18 with its stored
setup data. This supports another setup attempt after a notified change.
The category-1 state-4 path above passes false, so it does not itself
take this notification route.

## Packed resource child construction

`PackedResourceNodeFactory` vtable `0x00FB6E68` slots 1, 2 and 3 point to
`0x00A3D700`, `0x00A3D880` and `0x00A3D7D0`. These construct a
`ResourceLoader::ResourceData` reader through `0x00E0D450` or `0x00E0D4C0`
and pass it to `0x00A3BF10`. The reader installs vtable `0x0113BA20`.
Its initializer `0x00E0D120` points reader `+0x18` at resource buffer
`+0x30` and reader `+0x1C` at buffer `+0x40`. Instructions
`0x00E0D274` through `0x00E0D27F` set the content base after the count's
16-byte directory entries. Reader slot 1 returns the count less reserved
entries; slots 3 and 6 obtain an entry's content pointer and size.

For the directory, factory and root observations below, pefile 2024.8.26
mapped the following `.text` intervals and Capstone 5.0.7 decoded them in
32-bit x86 mode. The source was the 15,996,808-byte retail executable
identified above. End addresses are exclusive; file offsets equal VA minus
`0x00400000` for these selected intervals.

| Selected VA interval | Bytes / instructions | Code SHA-256 |
| --- | ---: | --- |
| `0x00A3BF10..0x00A3C1E6` | 726 / 261 | `76a460fc88d863d152e2cdd6283f14159dd1049a3986e044a3e9abea17124553` |
| `0x00A44220..0x00A44332` | 274 / 86 | `b5162aff45255361c717fbbca39b19fcee7ea4bede5de24e2a45e4f7bd3b9911` |
| `0x00A44A60..0x00A44AFD` | 157 / 42 | `15a37463f0e3c3533eaad262bd428cd9ae30507ef1b642721fa988f668bc5521` |
| `0x00A443A0..0x00A44651` | 689 / 201 | `50c5e69234d6b640f8dd19137323d474677cb78cebd3a0c470e617c23d706784` |
| `0x00A44660..0x00A44730` | 208 / 71 | `6f70291da40ce8219bc5b6f65134d70dec8df1359c6c4eb069225336a364533a` |
| `0x00A3DB40..0x00A3DC12` | 210 / 75 | `f714f8fbde6096f55cd67433af45a6ab25aff5d540f8a692097474d69f2f3c07` |
| `0x00A440F0..0x00A44161` | 113 / 34 | `f035ede5b292ca6c65e6d909ec727495ae75b59facdfd208dc31f23404b06698` |
| `0x00A44B00..0x00A44B69` | 105 / 39 | `3be84050d552bc5febe06a6be1b509ce5db9dd0cc3a4d6ea9b57ade5bde24229` |
| `0x00A465E0..0x00A466DD` | 253 / 81 | `1f911459fc45bb24e05bf4b1a309654b16a0b1d84c0335b1d79c32c167204053` |
| `0x00A46380..0x00A4645C` | 220 / 72 | `31b9f52227f9339b578cd1e580c2b98af9cb8bee033bc812a987057017a6c5a5` |

The selected four-byte data cell `0x0109A694..0x0109A698` held
`0x00A465E0`, matching the [slot catalog](../../config/ffxivgame.vtable_slots.jsonl)
row 64065. The cell binds the default callback target independently of the
catalog's `DefaultResourceNodeFactory` label.

### Raw directory arguments and callbacks

On the raw-entry route, `0x00A3C196` calls wrapper `0x00A44660` with
the seven arguments below. Reader offsets denote byte offsets in its
vtable. The [reader contract](gtex-pwib-loader.md#native-consumer-boundary)
owns the directory, metadata and endian interpretation. Each value means
its contents at the actual load, with saved-value identity conditional on
intact local/argument storage and expected ABI behavior.

| Argument | Producer | Reached factory role |
| --- | --- | --- |
| A1 | Current saved root R, loaded at `0x00A3C185` | Parent/append input |
| A2 | Name pointer originally from reader `+0x1C` at `0x00A3C109` | String copied locally and split into its last path component |
| A3 | Pointer to two reader `+0x24` ID qwords, selectors 0/1 at `0x00A3C035/47`, each byte-reversed with qword order preserved | Supplied 16-byte ID pair for `C+0x20..+0x2C` |
| A4 | Pointer to local `RESOURCE_TYPE`, originally reader `+0x20` at `0x00A3C025` | Wrapper lookup input and callback type-pointer argument |
| A5 | Payload pointer from reader `+0x0C` at `0x00A3C160` | Raw callback pointer input |
| A6 | Current saved byte length, originally reader `+0x18` at `0x00A3BFEF`, loaded at `0x00A3C175` | Callback length and `C+0x34` low 31-bit source |
| A7 | Fresh context from reader `+0x14` at `0x00A3C173` | Nullable branch/callback context |

The wrapper supplies current A4 to lookup `0x00A44B00` at `0x00A4469A`,
then forwards its seven current slots in order to `0x00A443A0` at
`0x00A44714`, adding A8=1 and incoming EAX=0. A nonzero reader view instead
selects the opaque `0x00A44730` call at `0x00A3C151`; its body is not
established by this raw-wrapper trace. The supplied selector's low byte
is set from initial context byte `+0x14`; those stores do not initialize
the pushed dword's upper bytes.

In factory `0x00A443A0`, let B be entry ESP minus `0x118`, F incoming
ECX, V incoming EAX, and L the last component of the local name copy at
B+0x18. The callback targets are loaded from F's vptr and use ECX=F.
When current A7 is nonzero, let T be the dword read at that pointer +0x10.
The earlier tests set BL on the reached T nonzero and [T+8] zero path.
That branch has priority.
Every numbered argument below is the then-current slot value at its load.

| Reached condition / callsite | Callback byte offset | Arguments in callee order | Selected child C |
| --- | --- | --- | --- |
| BL nonzero; `0x00A44576` | `+0x08` | A4, L, A5, A6, A7 | Full callback EAX |
| BL zero, current A8 low byte nonzero; `0x00A445BE` | `+0x04` | Address B+0x14, A4, L, A5, A6, A7 | Dword at callback EAX |
| BL zero, current A8 low byte zero; `0x00A445FB` | `+0x0C` | A4, L, V, A6, A7 | Full callback EAX |

Thus A4 is callback argument 1 on `+0x08/+0x0C`, and argument 2 on
`+0x04`. Only `+0x08` has the local zero-C return branch. The `+0x04`
route clears the dword at callback EAX, not a demonstrated child field,
then conditionally cleans up the object in local B+0x14. Its alias
relationship to the callback's returned pointer is unproved for an arbitrary
callback; the qualified default relation is resolved below.

The name-length check can call an opaque error target for length at least
`0x100`. If that call returns, the visible copy still has no bound and can
overwrite saved state and argument slots. Later A1/A3..A8 loads therefore
need intact slots, a suitable copy extent, no intervening alias mutation,
and expected stack/register/return state to equal their entry values.
Opaque error calls are not assumed to abort or preserve reachable memory.

At `0x00A4461F`, ESI=C and EDI=current A1 enter `0x00A440F0`. Let C0
and P0 be that function's entry ESI and EDI. Its body explicitly assigns
neither register. Every normal path sets full EAX=current ESI at
`0x00A44154`, overwriting callee EAX. Entry-value identity on calling paths
requires reached callees to preserve ESI/EDI and the expected stack/return state.

| Entry condition | Reached calls under preserved registers | Full EAX on normal return |
| --- | --- | --- |
| P0=0, any C0 | None | C0, with no callee preservation premise |
| P0 nonzero, C0=0 | Append `0x00A3B950` | 0 under the reached-call ABI/state conditions |
| P0 and C0 nonzero | Opaque `0x00A3E670` twice, append `0x00A3B950`, opaque `0x00A3E680` | C0 under the reached-call ABI/state conditions |

The append call at `0x00A44138` uses ECX=current EDI and one child word
written from ESI at `0x00A4412B`, before the second opaque helper.
Append-time child identity also requires that stack word to remain intact.
A nonzero parent does not guard append against a zero child. Successful
accesses, returning calls and intact stack/return state remain conditions;
no null safety, ownership or lifetime guarantee follows.

The reached append helper `0x00A3B950` detaches an already-parented child
through `0x00A3B820`, writes child `+0x14` to the new parent, and fills
parent `+0x18` or appends through sibling `+0x1C` links. A parent-zero
return therefore need not establish any tree membership. This construction
mechanism remains conditional on reached entries, callbacks and allocations.

The current A3 pointer reaches `0x00A3DB40` at `0x00A4462E` with ECX=C.
Its reached copy interleaves four dword loads with stores to
`C+0x20/+0x24/+0x28/+0x2C`; it is neither a snapshot nor an overlap-safe
copy. It has no direct `C+0x30` operand. At `0x00A44643`, the factory
instead updates `C+0x34` to `(old & 0x80000000) | (current A6 & 0x7FFFFFFF)`.
Neither operation is a child-type writer. The wrapper preserves actual
factory EAX, which is zero on the `+0x08` null-child path and otherwise
the final helper's EAX, not a proved C return. On copy paths that helper
returns the exact third-dword load at `0x00A3DBFE`; an opaque nonzero-AL
alternative preserves that call's full EAX.

### Type lookup and default raw creator

At lookup `0x00A44B00`, let M be entry ECX, O the output-pair address and
T the current type pointer. The wrapper supplies M=its receiver+4 and T=A4.
The lookup reads H0=[M+4] and the head's `+4` node pointer. A terminal-byte
test at node `+0x15` can skip the key dereference. Otherwise it loads k0=[T]
and walks signed node keys at `+0x0C`: key<k0 follows `+8`, while the other
edge retains that node as candidate D and follows `+0`. This is a lower-bound
search under valid ordered-tree, sentinel and stable-memory invariants.

The final test freshly loads H1=[M+4]. D==H1 selects pair (M,H1).
Otherwise a fresh k1=[T] with signed k1<[D+0x0C] also selects (M,H1);
the other edge selects (M,D). Stable k0=k1 and a valid ordered tree make
the candidate an exact-key match. The two components are loaded before
stores to O/O+4, and full EAX=O. The lookup makes no calls, insertion or
allocation. Output aliasing prevents an arbitrary map-nonmutation claim.

With intact output/slot and ABI state, K denotes the second component loaded
by the raw wrapper from the lookup's returned pair. Its signed nonpositive
current A6 route selects its local default. The positive route also selects
the default when K equals its fresh head read or a reached [K+0x10] is
zero; otherwise factory ECX comes from a fresh [K+0x10] load. Repeated
loads and returning diagnostic calls retain the qualifications above.
The registered value's provenance is not established by the lookup.

The [ResourceFactory registration contract](resource-factory-registration.md)
joins the outer owner to the then-current inner map and conditionally joins
registration inputs to node key/value storage. It does not establish an active
`0x63696262` entry or the factory bound to a particular lookup.

The local fallback initially has vptr `0x0109A690`. If it remains intact,
the authenticated byte+4 cell selects `0x00A465E0` on the qualified raw
`+0x04` branch. BL's `+0x08` route still has priority. This callback's
six arguments are `(output address, type pointer, L, raw pointer, length,
context)`. Both allocation paths request `0x50` bytes (80 decimal), not
a universal object size. A nonzero context path can call virtual `+8`;
the fallback uses opaque allocation helpers. Their semantics remain unbound.

For a nonzero allocation Z, both paths call `0x00A46380` at
`0x00A46643/0x00A466BB` with ECX=Z and current arguments
`(L, type pointer, raw pointer, length)`. Zero allocation supplies result 0.
Each normal callback path stores its result through the then-current output
slot and returns that output address in full EAX. With intact output slot,
storage and return state, the factory's following dereference obtains that
result C and its clear targets the same output local. This alias is resolved
only for this concrete default callback, not arbitrary registered callbacks.

The constructor retains Z in ESI under expected ABI behavior. It installs
cataloged `SQEX::CDev::Engine::Dw::SceneGraph::Resource::DefaultResourceNode`
primary vptr `0x0109AA8C` at full `+0` and secondary `0x0109AA80` at
full `+0x0C` at `0x00A463B9/BF`. These stores and the class label do not
identify a current runtime object or give either interface a semantic name.
Opaque calls and local/argument mutation remain relevant to base identity.

After the opaque name-related call, `0x00A463D2` retains the current length
in EDI; `0x00A463D6/DA` loads the current type pointer and its dword k into
EDX. The intervening low-31-bit update of full `+0x34` preserves its old
high bit and does not change EDX. `0x00A463E6` stores that exact loaded k
to current ESI+0x30, with no intervening call. Even an alias between the
length-update destination and type source does not change the already loaded
k. Under the base/output/ABI premises this is the selected child C's type
store; subsequent opaque calls can change the field again.

### Default child +0x40 value source

Let X denote the constructor's current ESI at each access, conditionally Z
and the published child C. Its full-base `+0x40` operations are:

| VA | Exact reached operation |
| --- | --- |
| `0x00A463A3` | Write dword [X+0x40]=0 before the opaque name-related call |
| `0x00A463E9..0x00A463FC` | Load G=[0x01327C24]; call dword[dword[G]+0x0C] with ECX=G and `(retained length, 0x10, 0x0109AA5C)` |
| `0x00A46400` | Store that call's full EAX U0 to [X+0x40] |
| `0x00A46405..0x00A4642A` | Nonzero U0 enables its virtual +0x18; a nonzero result enables fresh U1=[X+0x40], another virtual +0x18 and the returned object's virtual +8; full EAX==3 enables OR mask `0x01` into byte [X+0x38] |
| `0x00A4642E..0x00A4643F` | Common path freshly loads U2=[X+0x40], invokes its virtual +4 at `0x00A46436`, then calls opaque `0x009D4600` with `(last virtual EAX, current raw pointer, retained length)` |

U0 is the exact store value, not an assumed allocation base or byte pointer.
U1/U2 need not equal U0 across opaque calls or aliases. Earlier null tests
do not guard the common U2 dereference. The later full EAX=current ESI
return at `0x00A4644C` requires expected saved-register/stack state.
Neither safe failure handling nor persistence of `+0x30/+0x40` follows.

The [separate SCB path](timeline-scb-resource-binding.md#cache-miss-construction)
also obtains a memory view through global `0x01327C24` slot 3. It supplies
an interface lead, not the current G, U2, their vptrs, backing bytes or lifetime
here. No byte-copy contract is inferred merely from the `0x009D4600` address.

### View directory arguments and default creator

The view route at `0x00A3C151` calls wrapper `0x00A44730`. The following
complete functions were checked against the same retail PE identified above.
Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 supplied the PE mapping
and x86-32 decoding. End addresses are exclusive; these file offsets equal
VA minus `0x00400000`.

| Selected VA interval | Bytes / instructions | Code SHA-256 |
| --- | ---: | --- |
| `0x00A44730..0x00A4485F` | 303 / 108 | `9ec8836126441ca948d6f900177f6d2c2b864681d5c53cc2a08d0238a3b98e1a` |
| `0x00A466E0..0x00A467A7` | 199 / 64 | `3b62356fc00f56c2aa34875fe386918f265fb466b17a8aa40b4b12c62f803398` |
| `0x00A46520..0x00A465E0` | 192 / 58 | `7ca5740adc23a9541505671012deb2b127721fda2fc4eccb88becc05716c8338` |

Three exact four-byte cells were authenticated against that PE and the
[slot catalog](../../config/ffxivgame.vtable_slots.jsonl), rows
64067/64069/64070:

| Cell VA | Stored pointer | Default factory role |
| --- | --- | --- |
| `0x0109A69C` | `0x00A466E0` | Slot `+0x0C` |
| `0x0109A6A4` | `0x00B73290` | Predicate slot `+0x14` |
| `0x0109A6A8` | `0x00B73290` | Predicate slot `+0x18` |

The [selected RES/txb finding](gtex-pwib-loader.md#selected-restxb-consumer)
owns the true-return contract for `0x00B73290`. These cells qualify the
local default route, without identifying a registered CIBB callback.

At wrapper entry, ECX is factory/map owner P. Let B0 be entry ESP minus
`0x2C`, after the initial frame and three register saves. Its seven inputs
are:

| Slot | B0-relative offset | Reached role |
| --- | --- | --- |
| W1 | `+0x30` | Parent/root value |
| W2 | `+0x34` | Name pointer |
| W3 | `+0x38` | Pointer to reversed ID pair |
| W4 | `+0x3C` | Pointer to local type dword |
| W5 | `+0x40` | Entry view |
| W6 | `+0x44` | Fresh parser context |
| W7 | `+0x48` | Selector-containing dword |

Only W7's low byte was initialized by this caller: initially 1 when its
fetched context and context byte `+0x14` were nonzero, otherwise 0. Each
value below means current slot contents at its actual load. Entry/saved
identity requires intact storage and expected call/ABI behavior.

At `0x00A4474B`, EDI receives current W5 as V0. For nonzero V0, virtual
`+8` at `0x00A4475E` returns L0, then virtual `+4` at `0x00A4476B`
returns D0. Their locals are at B0+`0x0C` and B0+`0x10`. Zero V0 sets
both locals from preserved zero EBX. These calls do not establish the getter
bodies, backing bytes, bounds or one coherent view/pointer/length snapshot.

After PUSH EBP changes the base to B=B0-4, the wrapper initializes its
local default vptr `0x0109A690` at B+`0x18`. It calls lookup `0x00A44B00`
at `0x00A44797` with M=P+4, output-pair address B+`0x1C` and current W4.
It dereferences lookup EAX without a null guard. The lookup and
[registration contract](resource-factory-registration.md) retain their
existing conditions. With intact lookup/output/ABI state, the returned pair
contains M and candidate or head K.

The `0x00A4479C` unsigned comparison of the current L0 local against
preserved initial EBX=0 selects the default for zero length. Otherwise the
candidate/head/value tests, possible returning diagnostics `0x009D22B4`,
and repeated loads can select a fresh [K+`0x10`] factory pointer F. A zero
value selects the default receiver, address B+`0x18`. An earlier nonzero
test does not guarantee the later selected load is nonzero. A high-bit
length is not a signed nonpositive-length fallback here.

For selected F, the wrapper freshly loads its vptr for predicate `+0x14`
at `0x00A447EE`, saves that call's AL in wrapper BL, then reloads the vptr
for predicate `+0x18` at `0x00A447F9`. Let p14/p18 mean the respective
AL!=0 tests. The outgoing W7 low byte is 0 exactly when the current input
low byte is 0 and both p14 and p18 are nonzero; otherwise it is 1. Only
that byte is written, so the upper three bytes remain current/unestablished.
Reached calls can mutate F, its vptr and slots. With the default local/vptr
preserved, the authenticated predicates therefore leave an input-low-zero
default route at zero and otherwise produce one.

At `0x00A44815..0x00A44843`, the common factory receives
`(current W1, current W2, current W3, current W4, current D0 local,
current L0 local, current W6, current W7 dword after the byte update)`.
Incoming EAX is fresh W5 from `0x00A4483B`, denoted Vf, and ECX is saved
F. Vf need not equal V0 or describe the earlier D0/L0 observations.

Common factory `0x00A443A0` has its own BL mode from current context
`+0x10` and that object's `+8` test. This is distinct from wrapper predicate
BL. Its `+0x08` route has priority; only after excluding it does current
argument-8 low byte select `+0x04` when nonzero and `+0x0C` when zero.
The qualified `+0x0C` callback receives `(current type pointer, last name
component, saved incoming view V, current length, current context)`, with
V's identity conditional on preceding preservation. The common factory's
unbounded name copy, current slots and opaque-call conditions still apply.

The authenticated default `+0x0C` callback is `0x00A466E0`. It captures
its fifth argument, context, in EDI at `0x00A466F9`. If context, its
`+0x10` pointer and that pointer's `+8` dword are nonzero, virtual `+8`
receives a `0x50`-byte request. The fallback through opaque allocation
helpers `0x0040E2D0`, optional `0x0040E500` and `0x0040E110` also
requests `0x50` bytes (80 decimal). This request is not a universal sizeof
claim or a guarantee of allocation/failure safety.

Under expected cleanup, fallback tag store `0x00A4673F` overwrites callback
argument slot 5, without itself changing captured EDI. Nonzero allocation Z
reaches constructor `0x00A46520` with ECX=Z and
`(current name, current type pointer, current view, current length,
captured EDI context)` at `0x00A46765..0x00A4677C`. The fifth input is
not a fresh load of the overwritten slot. Its saved identity still requires
EDI preservation across reached calls. Zero allocation returns full EAX=0
without calling the constructor. Otherwise the callback preserves the
constructor's actual full EAX through its normal `ret 0x14` epilogue.
There is no output-address argument or output-local extraction/clear here.

The constructor saves allocation Z in ESI. Its current five arguments are
`(name, type pointer, view, length, context)`; this body has no explicit
argument-5 read. Opaque `0x00632D30` and name-related `0x00A3DC20`,
slot/memory preservation and expected cleanup qualify the source identities.
Its direct operations include:

| VA | Reached operation |
| --- | --- |
| `0x00A46542` | Write dword [current ESI+`0x40`]=0 |
| `0x00A46558/5E` | Install `0x0109AA8C` at full base and `0x0109AA80` at full `+0x0C` |
| `0x00A46565` | Write byte [ESI+`0x48`]=1 before the opaque name-related call |
| `0x00A46571` | Load current length L for the `+0x34` update |
| `0x00A46575/79` | Load current type pointer, then its dword k into EDX |
| `0x00A4657B` | Load current view argument into ECX as U0 |
| `0x00A46584` | Complete the low-31-bit XOR update at full `+0x34` |
| `0x00A46589` | Write captured k to dword [ESI+`0x30`] |
| `0x00A4658C` | Write captured U0 bits directly to dword [ESI+`0x40`] |

These are the same cataloged `DefaultResourceNode` tables identified above.
They do not establish a live type/vptr observation. No call intervenes
between the k/U0 loads and their stores. Earlier length-field writes can
alias the source storage, but the two values are already in registers.
The byte `+0x48` write applies to this constructor path; later opaque calls
can change it and the other fields.

For `+0x34`, let d0 be the first field read and d1 its value at the final
memory XOR. The exact result is
`d1 ^ ((d0 ^ L) & 0x7FFFFFFF)`. Interpreting it as preserving the high bit
and replacing the low 31 bits with L requires stability of those low bits
between d0 and d1. This is not an atomic field snapshot.

Nonzero U0 receives virtual `+0x18` at `0x00A46596`, returning J0. Nonzero
J0 enables fresh U1=[ESI+`0x40`] at `0x00A4659C` and another virtual
`+0x18` at `0x00A465A4`. Its result J1 is dereferenced without another
null guard. J1's virtual `+8` at `0x00A465AD` returns a full discriminator;
equality to 3 ORs `0x01` into byte [ESI+`0x38`]. Other paths do not
directly clear that byte. U1 and J1 need not equal earlier observations.

All normal constructor paths return full EAX=current ESI without adjustment
and `ret 0x14`. Under the saved-base and call/state conditions, this is Z
and the concrete callback's child C. The common `+0x0C` branch takes C
directly from actual callback EAX. The wrapper preserves the common factory's
actual final EAX through `ret 0x1C`; the later ID helper still defines that
final value on its continuation paths, so it is not an unconditional C return.

This constructor has neither the raw constructor's call through global
`0x01327C24` nor its `0x009D4600` call. The direct view-bit store proves
no backing copy, valid byte span, retain, ownership, lifetime or persistent
field value. The view interfaces remain opaque. Actual CIBB dispatch, the
C=N join and the distinction between root Q, child C and map owner P retain
the limits above.

### Root constructor and conditional full-object +0x40

Main `0x00A3BF10` initially reads header scalar H through reader `+0x28`
at `0x00A3BF2C`, from header `+0x3C`. Its calls to creator `0x00A44220`
at `0x00A3BF72/0x00A3BFB1` supply `(M1, M2, &local H, 0 or initial context)`,
where M1/M2 are the first two main entry arguments. Both allocation paths
request `0x48` bytes. Nonzero allocation reaches constructor `0x00A44A60`
with allocation base Q in ECX and current `(M2, &local H)` as arguments.

The constructor installs `0x00FBC32C` at Q and `0x00FBC31C` at Q+0x0C
at `0x00A44A94/9A`. Its first supplied pointer reaches the helper at
`0x00A44AA1` for the four-dword copy to Q+0x20..+0x2C. At `0x00A44AA6/AA`, it loads
the current second-argument pointer and then its current pointed-to dword J.
`0x00A44AAC` writes J to Q+0x30. Equality J=the original H requires intact
arguments, local/pointee state, alias conditions and expected ABI behavior.
A later opaque `0x00A3DC20` call may change that field, with no reread.
The constructor separately writes Q+0x40=0 at `0x00A44AE5` and returns
EAX=Q at `0x00A44AED`, without adjustment under the same ABI conditions.
The [separate constructor](attribute-resource-and-scene-binding.md) at
`0x00632F20` writes fixed `trsr`; that literal is not universal to roots.
The loop's per-entry type pointer is separate from this header-derived source.

Constructor result Q enters the creator's append input; zero allocation
supplies zero instead. Let U be actual append EAX. The explicit append
return supports U=Q under the reached-call register/stack conditions above.
The creator preserves U directly or through ESI and main saves it as R.
Thus Q=U=R is conditional on all intervening ABI, local-storage and return
conditions. A null allocation supplies U=R=0 only if the whole reached
creator path returns normally, without proving safe failure handling.
The factory's append result similarly equals selected C only conditionally;
the later ID helper still defines the factory's final continuation EAX.
Root Q, append U, saved R, factory child C and searched N retain these
separate roles; the qualified Q=U=R composition supplies no C=N join.

At main's tail, P denotes the current saved context-derived pointer,
initially zero or the earlier load from initial context `+0x10`. It loads
W=[P+8] once at `0x00A3C1B4`. The store at `0x00A3C1CF` writes that saved
W unchanged to `[EBX+0x40]` only after P and W are nonzero, current M1
equals current `[P+0x10]`, and `(current [P+0x0C] - W)` modulo 2^32
equals current `[P+4]`. These loads are not an atomic snapshot. EBX=R
requires intact saved local/register state and valid accessed memory.
Main ignores wrapper EAX, restores its current saved root after each reached
wrapper, and returns actual EBX bits. Where R=Q holds, this conditional
store shares the constructor's Q base; neither Q+0x30 nor Q+0x40 persistence
is established by these reached writes.

For type k=`0x63696262`, the qualified default route supports a reached
write of that loaded value to child `+0x30`. It does not establish that an
actual CIBB resource selects the default rather than a registered callback.
The exact map record/factory at [K+0x10] for that key, C=N pointer/layout
join, global G/view-getter implementations and backing remain unbound.
Root Q and child C are separate. No actual resource tree, actor/getter/interface
pointer, serialized CIBB identity or encounter identity follows.

The relevant exported listings are under `asm/ffxivgame/`, including
`0063bf10_FUN_00a3bf10.s`, `006443a0_FUN_00a443a0.s`,
`0063b950_FUN_00a3b950.s` and `00a0d120_FUN_00e0d120.s`.
Their instruction bytes and both cataloged vtables were compared with the
original PE identified above.

## Separate Attribute physics path

The selected installed template `data/7F/1F/00/00.DAT`, 9,816 bytes,
SHA-256 `c0a0e6128d36946ba32f8980ad80d39fd3aec990f3fc8a18742bbd8851a76b48`,
declares the floor BGChip at file `0x1E60` and an Attribute at `0x1DCC`.
Their resource-name rows at file `0x120` and `0x140` declare `brt`/`bhp`
with IDs `0x7F1C0000`/`0x7F200000`. The Attribute's name pointer at file
`0x1DF0` contains payload-relative `0xA15`, selecting key
`22hJhCc0_00_p_h` at file `0xBA5`. That key's `bhp` row declares the
second ID. These are separate serialized bindings.
The selected Attribute file `data/7F/20/00/00.DAT`, 78,928 bytes,
SHA-256 `7d8696f2bf5ccbe715b609ab576705aa473a4db57e50655cc4238a0656cdf3b1`,
starts with `SEDBPHB\0` and has `PHB.GBD\0` at file `0xC0`.

The Attribute creator vtable `0x0103C288` slot 1, `0x00837D80`, constructs
the base object through `0x008D3D00`. Its main interface vtable
`0x0106118C` identifies `RaptureAttributeBaseObject`. In the block's binding
phase, its slot 27, `0x00AA7560`, tests the retained resource at interface
`+0x48`; slot 22, `0x00AA8030`, obtains resource index zero from the bind
entry and passes it to slot 39, `0x00AA7520`. This slot retains that
resource at `+0x48`. Slot numbers have receiver-specific meanings: Attribute
slot 39 is a setter, unlike the BGChip's resource-name getter.

`0x00AA8030` then passes the resource through `0x00AA7C70`. This helper
gets global `0x01365B08` slot 7, obtains a manipulator through the returned
manager's slot 1, builds a type-9 descriptor containing the resource at
descriptor `+0x0C`, and calls manipulator slot 14. If slot 34 reports a
result, it obtains it through slot 35, then releases the temporary through
slot 43. Back in `0x00AA8030`, the returned manager's slot 3 receives that
result and supplies Attribute interface `+0x4C`. This is a separate resource
consumer path from the model initializer's descendant search below.

The default manager's concrete identity follows from its construction:

| Interface | Construction and dispatch |
|---|---|
| `LayExternalObjectManager`, `0x010A8AC0`, full `+0x04` | `0x00AB70F0` constructs it through `0x00ABB9E0` and stores the interface in `0x01365B08`; slot 7 is `0x00ABB960` |
| `LayExternalManagerConnector`, `0x010A026C`, full `+0x04` | `0x00A77A30` constructs it through `0x00A841D0` and installs it through wrapper slot 1, `0x00ABB900`; wrapper slot 7 forwards connector slot 4, `0x00A841B0` |
| `LayPhysicsManager`, `0x0109EA3C`, full `+0x04` | `0x00A77A30` constructs it through `0x00A73850` and stores the full object through `0x00A73390`; connector slot 4 returns that global `0x0136568C` plus four through `0x00A733C0` |
| `LayRigidBodyManipulator`, `0x0109F984`, full `+0x04` | Physics-manager slot 1, `0x00A73650`, obtains an object through `0x00A73500` / `0x00A79550`; its slot 14 is `0x00A78FE0` |

The identities and adjustments were checked through PE RTTI locators and
cataloged slots. They qualify the default construction route; external
manager replacement or allocation failure can change the receiver.

The manipulator's jump table at `0x00A792B0` sends type 9 to
`0x00A79228`. That branch takes the descriptor's resource and prepares
resource `+0x44` plus a vector. It calls `0x00A6CC90`, which returns resource
`+0x48` when resource `+0x58` is null, or delegates to the latter object's
slot 1. The direct `+0x48` branch consumes no stack arguments. The two
prepared pushes remain for `0x00AC0350`, called at `0x00A79283` with the
backing physics pointer, returned PHB data, vector and resource `+0x44`.
That routine examines PHB structures and conditionally tail-calls
`0x00AC0B00`. The caller returns success only for a non-null result.
This establishes a conditional PHB physics construction consumer, without
requiring the Attribute to become a child of the primary render resource.

For placed-instance setup, Attribute slot 18, `0x008D3DB0`, first checks
instance slot 50 and then calls `0x00AA81D0`. That routine obtains the
instance property, checks category `0x0B`, and uses Attribute slot 52,
`0x00AA8480`, to build another type-9 descriptor from retained resource
`+0x48`. It applies the supplied transform, dispatches manipulator slots
18 and 24 with true arguments, and forwards the base-object flag byte through
slot 26. The result is attached through property slot 29. Property state,
admission, manager availability, resource retention and allocation success
remain conditions; these instructions do not demonstrate player walking.

The [Attribute registration trace](attribute-resource-and-scene-binding.md)
follows selected instance admission, native body/shape registration and the
exact placed-setup flag and group writes.

## Model collision consumer

The cataloged `RaptureModelObject` main vtable `0x010653A4` maps slot 1
to `0x00A61DF0` and slot 79 to `0x00A612E0`. Its initializer's loop at
`0x00A62180` reads candidate pointers from an array passed on the stack,
tests each through `0x00A6C500`, and sends matching candidates to slot 79
at `0x00A621AC`.

`0x00A6C500` calls `0x00A3D970` with the dword `0x00706862`, little-endian
`bhp\0`, and flag 1. That flag skips the candidate's own `+0x30` type
test. The search begins at candidate `+0x18`, follows `+0x18/+0x1C` links,
and compares visited nodes' `+0x30` values. This detects a `bhp` descendant.
It does not inspect the layout property's additional-resource array.

`0x00A612E0` lazily allocates `0xB0` bytes, calls `0x00A6C2E0` and retains
the controller at model `+0xF8`. That constructor installs
`SQEX::CDev::Engine::Fw::SceneObject::Controllers::PhysicsController`
vtable `0x0109DE84`. Its slot 1, `0x00A6CA70`, receives `(candidate, 0)`
when both controller and candidate are non-null. The controller obtains
resource-side information through `0x00A6C400`, creates proxies through
`0x00A6C9C0` and `0x00A6C530`, and recursively dispatches its slot 1 over
the child chain. Proxy constructors `0x00A6CDD0` and `0x00A6D880` install
the cataloged MeshProxy and JointProxy identities.

The connected fallback path supplies the primary resource to this consumer,
subject to the intervening guards and successful allocations. The selected
floor's primary key resolves to `brt7F1C0000`; its separately declared
Attribute is `bhp7F200000`, as recorded in the selected-file fields above.
A direct Python byte/field check
of installed `data/7F/1C/00/00.DAT`, 41,088 bytes, SHA-256
`1af91b0e1e5e34f676f1af2884f82eddb63062e62bee058b0260e9646dfb7d0f`, found
a `SEDBRES ` container with type `brt` and five directory entries. The
directory starts at file `0x40`, with content base `0x90`; the entries begin
at `0x90`, `0x221C`, `0x9E90`, `0x9FB4` and `0x9FD0`. None declares `bhp`,
and the complete file contains no ASCII `bhp` literal. This file-level
observation does not rule out a loader attaching a PHB child to the resource
tree. The selected Attribute-to-primary-resource child relation therefore
was not established by this trace. The Attribute has the separate physics
consumer described above.
These instructions do not establish successful rendering or player walking.
